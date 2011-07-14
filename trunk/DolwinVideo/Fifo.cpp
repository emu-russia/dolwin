// fifo parser engine
#include "GX.h"

//
// local data
//

static  u8      *(__fastcall *pipeline[VTX_MAX_ATTR][8])(u8 *ptr); // fifo attr callbacks

static  float   fracDenom[8][VTX_MAX_ATTR];             // fraction denominant

static  Vertex  *vtx;                                   // current vertex to 
                                                        // collect data
static  int     VtxSize[8];

static  unsigned usevat;                                // current VAT

static  FILE    *filog;                                 // fifo log

u32     lastFifoSize;

u8  accum[1024*1024+32];// primitive accumulation buffer
u8 *accptr;             // current offset in accum
u32 acclen;             // length of accumulated data
u8  gxcmd;              // next fifo command to execute
u32 need;               // need bytes for command
u8  cmdidle=1;

// ---------------------------------------------------------------------------

// stage callbacks

#include "Stages.cpp"       // exclude from build !
#include "Stages.h"

// helper function
// (no need anymore)
static char *getAttrDesc(VTX_ATTR attr)
{
    switch(attr)
    {
        case VTX_POSMATIDX:     return "Position Matrix Index";
        case VTX_TEX0MTXIDX:    return "Texture Coordinate 0 Matrix Index";
        case VTX_TEX1MTXIDX:    return "Texture Coordinate 1 Matrix Index";
        case VTX_TEX2MTXIDX:    return "Texture Coordinate 2 Matrix Index";
        case VTX_TEX3MTXIDX:    return "Texture Coordinate 3 Matrix Index";
        case VTX_TEX4MTXIDX:    return "Texture Coordinate 4 Matrix Index";
        case VTX_TEX5MTXIDX:    return "Texture Coordinate 5 Matrix Index";
        case VTX_TEX6MTXIDX:    return "Texture Coordinate 6 Matrix Index";
        case VTX_TEX7MTXIDX:    return "Texture Coordinate 7 Matrix Index";
        case VTX_POS:           return "Position";
        case VTX_NRM:           return "Normal or Normal/Binormal/Tangent";
        case VTX_COLOR0:        return "Color 0";
        case VTX_COLOR1:        return "Color 1";
        case VTX_TEXCOORD0:     return "Texture Coordinate 0";
        case VTX_TEXCOORD1:     return "Texture Coordinate 1";
        case VTX_TEXCOORD2:     return "Texture Coordinate 2";
        case VTX_TEXCOORD3:     return "Texture Coordinate 3";
        case VTX_TEXCOORD4:     return "Texture Coordinate 4";
        case VTX_TEXCOORD5:     return "Texture Coordinate 5";
        case VTX_TEXCOORD6:     return "Texture Coordinate 6";
        case VTX_TEXCOORD7:     return "Texture Coordinate 7";
        case VTX_MAX_ATTR :     return "MAX attr";
    }
    return "unknown";
}

// calculate size of current vertex
static int gx_vtxsize(unsigned v)
{
    int vtxsize = 0;
    static int cntp[]   = { 2, 3 };
    static int cntn[]   = { 3, 9 };
    static int cntt[]   = { 1, 2 };
    static int fmtsz[]  = { 1, 1, 2, 2, 4 };
    static int cfmtsz[] = { 2, 3, 1, 2, 4, 4 };

    if(cpRegs.vcdLo.pmidx)  vtxsize++;
    if(cpRegs.vcdLo.t0midx) vtxsize++;
    if(cpRegs.vcdLo.t1midx) vtxsize++;
    if(cpRegs.vcdLo.t2midx) vtxsize++;
    if(cpRegs.vcdLo.t3midx) vtxsize++;
    if(cpRegs.vcdLo.t4midx) vtxsize++;
    if(cpRegs.vcdLo.t5midx) vtxsize++;
    if(cpRegs.vcdLo.t6midx) vtxsize++;
    if(cpRegs.vcdLo.t7midx) vtxsize++;
    
    if(cpRegs.vcdLo.pos & 2) vtxsize += cpRegs.vcdLo.pos - 1;
    if(cpRegs.vcdLo.nrm & 2) vtxsize += cpRegs.vcdLo.nrm - 1;
    if(cpRegs.vcdLo.col0& 2) vtxsize += cpRegs.vcdLo.col0- 1;
    if(cpRegs.vcdLo.col1& 2) vtxsize += cpRegs.vcdLo.col1- 1;
    if(cpRegs.vcdHi.tex0& 2) vtxsize += cpRegs.vcdHi.tex0- 1;
    if(cpRegs.vcdHi.tex1& 2) vtxsize += cpRegs.vcdHi.tex1- 1;
    if(cpRegs.vcdHi.tex2& 2) vtxsize += cpRegs.vcdHi.tex2- 1;
    if(cpRegs.vcdHi.tex3& 2) vtxsize += cpRegs.vcdHi.tex3- 1;
    if(cpRegs.vcdHi.tex4& 2) vtxsize += cpRegs.vcdHi.tex4- 1;
    if(cpRegs.vcdHi.tex5& 2) vtxsize += cpRegs.vcdHi.tex5- 1;
    if(cpRegs.vcdHi.tex6& 2) vtxsize += cpRegs.vcdHi.tex6- 1;
    if(cpRegs.vcdHi.tex7& 2) vtxsize += cpRegs.vcdHi.tex7- 1;

    if(cpRegs.vcdLo.pos ==1) vtxsize += fmtsz[cpRegs.vatA[v].posfmt] * cntp[cpRegs.vatA[v].poscnt];
    if(cpRegs.vcdLo.nrm ==1) vtxsize += fmtsz[cpRegs.vatA[v].nrmfmt] * cntn[cpRegs.vatA[v].nrmcnt];
    if(cpRegs.vcdHi.tex0==1) vtxsize += fmtsz[cpRegs.vatA[v].tex0fmt] * cntt[cpRegs.vatA[v].tex0cnt];
    if(cpRegs.vcdHi.tex1==1) vtxsize += fmtsz[cpRegs.vatB[v].tex1fmt] * cntt[cpRegs.vatB[v].tex1cnt];
    if(cpRegs.vcdHi.tex2==1) vtxsize += fmtsz[cpRegs.vatB[v].tex2fmt] * cntt[cpRegs.vatB[v].tex2cnt];
    if(cpRegs.vcdHi.tex3==1) vtxsize += fmtsz[cpRegs.vatB[v].tex3fmt] * cntt[cpRegs.vatB[v].tex3cnt];
    if(cpRegs.vcdHi.tex4==1) vtxsize += fmtsz[cpRegs.vatB[v].tex4fmt] * cntt[cpRegs.vatB[v].tex4cnt];
    if(cpRegs.vcdHi.tex5==1) vtxsize += fmtsz[cpRegs.vatC[v].tex5fmt] * cntt[cpRegs.vatC[v].tex5cnt];
    if(cpRegs.vcdHi.tex6==1) vtxsize += fmtsz[cpRegs.vatC[v].tex6fmt] * cntt[cpRegs.vatC[v].tex6cnt];
    if(cpRegs.vcdHi.tex7==1) vtxsize += fmtsz[cpRegs.vatC[v].tex7fmt] * cntt[cpRegs.vatC[v].tex7cnt];

    if(cpRegs.vcdLo.col0==1) vtxsize += cfmtsz[cpRegs.vatA[v].col0fmt];
    if(cpRegs.vcdLo.col1==1) vtxsize += cfmtsz[cpRegs.vatA[v].col1fmt];

    return vtxsize;
}

//
// parser reconfiguration routine
// see also Stages.h for details
//

void FifoReconfigure(
    VTX_ATTR    attr,       // stage attribute
    unsigned    vat,        // vat number
    unsigned    vcd,        // attribute description
    unsigned    cnt,        // attribute "cnt"
    unsigned    fmt,        // attribute "fmt"
    unsigned    frac)       // fraction
{
    switch(attr)
    {
        case VTX_POS:
        {
            // not allowed! pos should be always present
            //ASSERT(vcd == VCD_NONE);
            
            // set callback
            pipeline[VTX_POS][vat] = posattr[vcd][cnt][fmt];

            // calculate denominant (used to speed-up fixed-to-fp convertion)
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_POS] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "pos, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                     vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_NRM:
        {
            pipeline[VTX_NRM][vat] = nrmattr[vcd][cnt][fmt];

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "nrm, vat:%i, vcd:%i, cnt:%i, fmt:%i\n",
                                     vat,    vcd,    cnt,    fmt
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_COLOR0:
        {
            pipeline[VTX_COLOR0][vat] = col0attr[vcd][cnt][fmt];

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "col0, vat:%i, vcd:%i, cnt:%i, fmt:%i\n",
                                      vat,    vcd,    cnt,    fmt
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_COLOR1:
        {
            pipeline[VTX_COLOR1][vat] = col1attr[vcd][cnt][fmt];

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "col1, vat:%i, vcd:%i, cnt:%i, fmt:%i\n",
                                      vat,    vcd,    cnt,    fmt
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD0:
        {
            pipeline[VTX_TEXCOORD0][vat] = tex0attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD0] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex0, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD1:
        {
            pipeline[VTX_TEXCOORD1][vat] = tex1attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD1] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex1, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD2:
        {
            pipeline[VTX_TEXCOORD2][vat] = tex2attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD2] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex2, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD3:
        {
            pipeline[VTX_TEXCOORD3][vat] = tex3attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD3] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex3, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD4:
        {
            pipeline[VTX_TEXCOORD4][vat] = tex4attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD4] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex4, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD5:
        {
            pipeline[VTX_TEXCOORD5][vat] = tex5attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD5] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex5, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD6:
        {
            pipeline[VTX_TEXCOORD6][vat] = tex6attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD6] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex6, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        case VTX_TEXCOORD7:
        {
            pipeline[VTX_TEXCOORD7][vat] = tex7attr[vcd][cnt][fmt];

            // calculate denominant
            float denom = (float)pow(2.0, (double)frac);
            fracDenom[vat][VTX_TEXCOORD7] = denom;

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "tex7, vat:%i, vcd:%i, cnt:%i, fmt:%i, shft:%i\n",
                                      vat,    vcd,    cnt,    fmt,    frac
                );
                fflush(filog);
            }
#endif
        }
        break;

        // reset pipeline 
        case VTX_MAX_ATTR:
        {
            lastFifoSize = 0;

            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                for(unsigned attr=0; attr<VTX_MAX_ATTR; attr++)
                {
                    pipeline[attr][vatnum] = NULL;
                }
            }

            // create fifo log file
#ifdef  FIFOLOG
            if(filog)
            {
                fclose(filog);
                filog = NULL;
            }
            filog = fopen("fifolog.txt", "w");
#endif
        }
        return;

        case VTX_POSMATIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_POSMATIDX][vatnum] = pos_idx;
                else pipeline[VTX_POSMATIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set posidx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX0MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX0MTXIDX][vatnum] = t0_idx;
                else pipeline[VTX_TEX0MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX1MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX1MTXIDX][vatnum] = t1_idx;
                else pipeline[VTX_TEX1MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX2MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX2MTXIDX][vatnum] = t2_idx;
                else pipeline[VTX_TEX2MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX3MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX3MTXIDX][vatnum] = t3_idx;
                else pipeline[VTX_TEX3MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX4MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX4MTXIDX][vatnum] = t4_idx;
                else pipeline[VTX_TEX4MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX5MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX5MTXIDX][vatnum] = t5_idx;
                else pipeline[VTX_TEX5MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX6MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX6MTXIDX][vatnum] = t6_idx;
                else pipeline[VTX_TEX6MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        case VTX_TEX7MTXIDX:
        {
            for(unsigned vatnum=0; vatnum<8; vatnum++)
            {
                if(vcd > 0) pipeline[VTX_TEX7MTXIDX][vatnum] = t7_idx;
                else pipeline[VTX_TEX7MTXIDX][vatnum] = NULL;
            }

            // log output
#ifdef  FIFOLOG
            if(vcd > 0)
            {
                fprintf(filog, "set tx0idx\n");
                fflush(filog);
            }
#endif
        }
        return;

        default:
        {
            GFXError(
                "Fifo reconfigure failure! "
                "Unhandled vertex attribute %s.",
                getAttrDesc(attr)
            );
        }
        return;
    }

/*/
    // check for un-implemented stages
    if((pipeline[attr][vat] == NULL) && (vcd > 0))
    {
        GFXError(
            "Undefined vertex stage for %s\n"
            "vcd: %i, cnt: %i, fmt: %i, shft: %i",
            getAttrDesc(attr),
            vcd, cnt, fmt, frac
        );
    }
/*/

    for(unsigned v=0; v<8; v++) VtxSize[v] = gx_vtxsize(v);
}

// ---------------------------------------------------------------------------

// byte-swapping, used commonly 
// TODO : convert on asm to speed-up

u32 swap32(u32 data)
{
    return ((data >> 24) & 0x000000ff) |
           ((data >>  8) & 0x0000ff00) |
           ((data <<  8) & 0x00ff0000) |
           ((data << 24) & 0xff000000) ;
}

u16 swap16(u16 data)
{
    return ((data & 0x00ff) << 8) |
           ((data & 0xff00) >> 8) ;
}

// ---------------------------------------------------------------------------

// TODO : implement fifo, as _real_ fifo, not like that crappy one

// collect data
static u8 *FifoWalk(unsigned vatnum, u8 *ptr)
{
    register u8 *(__fastcall *stageCB)(u8 *ptr);

    // overrided by 'mtxidx' attributes
    xfRegs.posidx = xfRegs.matidxA.pos;
    xfRegs.texidx[0] = xfRegs.matidxA.tex0;
    xfRegs.texidx[1] = xfRegs.matidxA.tex1;
    xfRegs.texidx[2] = xfRegs.matidxA.tex2;
    xfRegs.texidx[3] = xfRegs.matidxA.tex3;
    xfRegs.texidx[4] = xfRegs.matidxB.tex4;
    xfRegs.texidx[5] = xfRegs.matidxB.tex5;
    xfRegs.texidx[6] = xfRegs.matidxB.tex6;
    xfRegs.texidx[7] = xfRegs.matidxB.tex7;

    for(unsigned stage=0; stage<VTX_MAX_ATTR; stage++)
    {
        stageCB = pipeline[stage][vatnum];
        if(stageCB) ptr = stageCB(ptr);
    }

    return ptr;
}

static u32 gx_needbytes(u8 cmd)
{
    u8 *readptr = accptr;

    switch(cmd)
    {
        case OP_CMD_NOP:
        case OP_CMD_INV:
            return 0;

        case OP_CMD_CALL_DL:
            return 8;

        case OP_CMD_LOAD_BPREG:
            return 4;

        case OP_CMD_LOAD_CPREG:
            return 5;
            
        case OP_CMD_LOAD_XFREG:
        {
            u16 len = swap16(*(u16 *)readptr) + 1;
            return 2 + 4 * len;
        }

        // 0x80
        case OP_CMD_DRAW_QUAD | 0:
        case OP_CMD_DRAW_QUAD | 1:
        case OP_CMD_DRAW_QUAD | 2:
        case OP_CMD_DRAW_QUAD | 3:
        case OP_CMD_DRAW_QUAD | 4:
        case OP_CMD_DRAW_QUAD | 5:
        case OP_CMD_DRAW_QUAD | 6:
        case OP_CMD_DRAW_QUAD | 7:
        case OP_CMD_DRAW_TRIANGLE | 0:
        case OP_CMD_DRAW_TRIANGLE | 1:
        case OP_CMD_DRAW_TRIANGLE | 2:
        case OP_CMD_DRAW_TRIANGLE | 3:
        case OP_CMD_DRAW_TRIANGLE | 4:
        case OP_CMD_DRAW_TRIANGLE | 5:
        case OP_CMD_DRAW_TRIANGLE | 6:
        case OP_CMD_DRAW_TRIANGLE | 7:
        case OP_CMD_DRAW_STRIP | 0:
        case OP_CMD_DRAW_STRIP | 1:
        case OP_CMD_DRAW_STRIP | 2:
        case OP_CMD_DRAW_STRIP | 3:
        case OP_CMD_DRAW_STRIP | 4:
        case OP_CMD_DRAW_STRIP | 5:
        case OP_CMD_DRAW_STRIP | 6:
        case OP_CMD_DRAW_STRIP | 7:
        case OP_CMD_DRAW_FAN | 0:
        case OP_CMD_DRAW_FAN | 1:
        case OP_CMD_DRAW_FAN | 2:
        case OP_CMD_DRAW_FAN | 3:
        case OP_CMD_DRAW_FAN | 4:
        case OP_CMD_DRAW_FAN | 5:
        case OP_CMD_DRAW_FAN | 6:
        case OP_CMD_DRAW_FAN | 7:
        case OP_CMD_DRAW_LINE | 0:
        case OP_CMD_DRAW_LINE | 1:
        case OP_CMD_DRAW_LINE | 2:
        case OP_CMD_DRAW_LINE | 3:
        case OP_CMD_DRAW_LINE | 4:
        case OP_CMD_DRAW_LINE | 5:
        case OP_CMD_DRAW_LINE | 6:
        case OP_CMD_DRAW_LINE | 7:
        case OP_CMD_DRAW_LINESTRIP | 0:
        case OP_CMD_DRAW_LINESTRIP | 1:
        case OP_CMD_DRAW_LINESTRIP | 2:
        case OP_CMD_DRAW_LINESTRIP | 3:
        case OP_CMD_DRAW_LINESTRIP | 4:
        case OP_CMD_DRAW_LINESTRIP | 5:
        case OP_CMD_DRAW_LINESTRIP | 6:
        case OP_CMD_DRAW_LINESTRIP | 7:
        case OP_CMD_DRAW_POINT | 0:
        case OP_CMD_DRAW_POINT | 1:
        case OP_CMD_DRAW_POINT | 2:
        case OP_CMD_DRAW_POINT | 3:
        case OP_CMD_DRAW_POINT | 4:
        case OP_CMD_DRAW_POINT | 5:
        case OP_CMD_DRAW_POINT | 6:
        case OP_CMD_DRAW_POINT | 7:
        {
            unsigned vtxnum = swap16(*(u16 *)readptr);
            return vtxnum * VtxSize[cmd & 7];
        }
    }

    return 1;
}

static void GPCallList(u8 *fifoPtr, u32 count)
{
    u8  *endptr  = &fifoPtr[count];
    u8  *readptr = fifoPtr;
    u8  cmd;

    while((u32)readptr < (u32)endptr)
    {
        switch(cmd = *readptr++)
        {
            // do nothing
            case OP_CMD_NOP:
            case OP_CMD_INV:
                break;

            case OP_CMD_CALL_DL:
                GFXError("Nested display list");
                readptr += 8;
                break;

            // ---------------------------------------------------------------
            // loading of internal regs
            
            case OP_CMD_LOAD_BPREG:
            {
                u32 word = swap32(*(u32 *)readptr);
                readptr += 4;
                loadBPReg(word >> 24, word & 0xffffff);
                break;
            }

            case OP_CMD_LOAD_CPREG:
            {
                u8 index = *readptr++;
                u32 word = swap32(*(u32 *)readptr);
                readptr += 4;
                loadCPReg(index, word);
                break;
            }

            case OP_CMD_LOAD_XFREG:
            {
                u16 reg, len, index;
                u32 *regData;

                len = swap16(*(u16 *)readptr) + 1;
                readptr += 2;
                index = swap16(*(u16 *)readptr);
                readptr += 2;

                // reverse bytes
                regData = (u32 *)readptr;
                for(reg=0; reg<len; reg++, readptr+=4)
                {
                    regData[reg] = swap32(regData[reg]);
                }

                loadXFRegs(index, len, regData);
                break;
            }

/*/
            case OP_CMD_LOAD_INDXA:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXB:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXC:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXD:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }
/*/

            // ---------------------------------------------------------------
            // draw commands

            // 0x80
            case OP_CMD_DRAW_QUAD | 0:
            case OP_CMD_DRAW_QUAD | 1:
            case OP_CMD_DRAW_QUAD | 2:
            case OP_CMD_DRAW_QUAD | 3:
            case OP_CMD_DRAW_QUAD | 4:
            case OP_CMD_DRAW_QUAD | 5:
            case OP_CMD_DRAW_QUAD | 6:
            case OP_CMD_DRAW_QUAD | 7:
            {
                static   Vertex   quad[4];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                    1---2       tri1: 0-1-2
                    |  /|       tri2: 0-2-3
                    | / |
                    |/  |
                    0---3
                                                            /*/
                while(vtxnum > 0)
                {
                    for(unsigned n=0; n<4; n++)
                    {
                        vtx = &quad[n];
                        readptr = FifoWalk(vatnum, readptr);
                    }
                    gfx->RenderTriangle(&quad[0], &quad[1], &quad[2]);
                    gfx->RenderTriangle(&quad[0], &quad[2], &quad[3]);
                    vtxnum -= 4;
                }
                break;
            }

            // 0x90
            case OP_CMD_DRAW_TRIANGLE | 0:
            case OP_CMD_DRAW_TRIANGLE | 1:
            case OP_CMD_DRAW_TRIANGLE | 2:
            case OP_CMD_DRAW_TRIANGLE | 3:
            case OP_CMD_DRAW_TRIANGLE | 4:
            case OP_CMD_DRAW_TRIANGLE | 5:
            case OP_CMD_DRAW_TRIANGLE | 6:
            case OP_CMD_DRAW_TRIANGLE | 7:
            {
                static   Vertex   tri[3];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                    1---2       tri: 0-1-2
                    |  /
                    | /
                    |/
                    0  
                                                            /*/
                while(vtxnum > 0)
                {

                    for(unsigned n=0; n<3; n++)
                    {
                        vtx = &tri[n];
                        readptr = FifoWalk(vatnum, readptr);
                    }
                    gfx->RenderTriangle(&tri[0], &tri[1], &tri[2]);
                    vtxnum -= 3;
                }
                break;
            }

            // 0x98 - not sure !
            case OP_CMD_DRAW_STRIP | 0:
            case OP_CMD_DRAW_STRIP | 1:
            case OP_CMD_DRAW_STRIP | 2:
            case OP_CMD_DRAW_STRIP | 3:
            case OP_CMD_DRAW_STRIP | 4:
            case OP_CMD_DRAW_STRIP | 5:
            case OP_CMD_DRAW_STRIP | 6:
            case OP_CMD_DRAW_STRIP | 7:
            {
                static   Vertex   tri[3];
                unsigned c = 2, order[3] = { 0, 1, 2 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                        1---3---5   tri1: 0-1-2
                       /|  /|  /    tri2: 1-2-3
                      / | / | /     tri3: 2-3-4
                     /  |/  |/      tri4: 3-4-5
                    0---2---4       ...
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 3);

                vtx = &tri[0];
                readptr = FifoWalk(vatnum, readptr);
                vtx = &tri[1];
                readptr = FifoWalk(vatnum, readptr);
                vtxnum -= 2;

                while(vtxnum-- > 0)
                {
                    vtx = &tri[c++];
                    readptr = FifoWalk(vatnum, readptr);
                    if(c > 2) c = 0;

                    gfx->RenderTriangle(
                        &tri[order[0]],
                        &tri[order[1]], 
                        &tri[order[2]]
                    );

                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = order[2];
                    order[2] = tmp;
                }
                break;
            }

            // 0xA0
            case OP_CMD_DRAW_FAN | 0:
            case OP_CMD_DRAW_FAN | 1:
            case OP_CMD_DRAW_FAN | 2:
            case OP_CMD_DRAW_FAN | 3:
            case OP_CMD_DRAW_FAN | 4:
            case OP_CMD_DRAW_FAN | 5:
            case OP_CMD_DRAW_FAN | 6:
            case OP_CMD_DRAW_FAN | 7:
            {
                static   Vertex   tri[3];
                unsigned c = 2, order[2] = { 1, 2 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                    1---2---3   tri1: 0-1-2
                    |  /  _/    tri2: 0-2-3
                    | / _/      trin: 0-[n-1]-n
                    |/_/    
                    0/
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 3);

                vtx = &tri[0];
                readptr = FifoWalk(vatnum, readptr);
                vtx = &tri[1];
                readptr = FifoWalk(vatnum, readptr);
                vtxnum -= 2;

                while(vtxnum-- > 0)
                {
                    vtx = &tri[c];
                    readptr = FifoWalk(vatnum, readptr);
                    c = (c == 2) ? (c = 1) : (c = 2);

                    gfx->RenderTriangle(
                        &tri[0],
                        &tri[order[0]],
                        &tri[order[1]]
                    );

                    // order[0] <-> order[1]
                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = tmp;
                }
                break;
            }

            // 0xA8
            case OP_CMD_DRAW_LINE | 0:
            case OP_CMD_DRAW_LINE | 1:
            case OP_CMD_DRAW_LINE | 2:
            case OP_CMD_DRAW_LINE | 3:
            case OP_CMD_DRAW_LINE | 4:
            case OP_CMD_DRAW_LINE | 5:
            case OP_CMD_DRAW_LINE | 6:
            case OP_CMD_DRAW_LINE | 7:
            {
                static   Vertex   v[2];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                        1   3   5
                       /   /   / 
                      /   /   / 
                     /   /   /      
                    0   2   4       
                                                            /*/
                if(vtxnum == 0) break;

                while(vtxnum > 0)
                {
                    vtx = &v[0];
                    readptr = FifoWalk(vatnum, readptr);
                    vtx = &v[1];
                    readptr = FifoWalk(vatnum, readptr);
                    gfx->RenderLine(&v[0], &v[1]);
                    vtxnum -= 2;
                }
                break;
            }

            // 0xB0
            case OP_CMD_DRAW_LINESTRIP | 0:
            case OP_CMD_DRAW_LINESTRIP | 1:
            case OP_CMD_DRAW_LINESTRIP | 2:
            case OP_CMD_DRAW_LINESTRIP | 3:
            case OP_CMD_DRAW_LINESTRIP | 4:
            case OP_CMD_DRAW_LINESTRIP | 5:
            case OP_CMD_DRAW_LINESTRIP | 6:
            case OP_CMD_DRAW_LINESTRIP | 7:
            {
                static   Vertex   v[2];
                unsigned c = 1, order[2] = { 0, 1 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                        1   3   5
                       /|  /|  / 
                      / | / | /  
                     /  |/  |/   
                    0   2   4    
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 2);

                vtx = &v[0];
                readptr = FifoWalk(vatnum, readptr);
                vtxnum--;

                while(vtxnum-- > 0)
                {
                    vtx = &v[c++];
                    readptr = FifoWalk(vatnum, readptr);
                    if(c > 1) c = 0;

                    gfx->RenderLine(
                        &v[order[0]],
                        &v[order[1]]
                    );

                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = tmp;
                }
                break;
            }

            // 0xB8
            case OP_CMD_DRAW_POINT | 0:
            case OP_CMD_DRAW_POINT | 1:
            case OP_CMD_DRAW_POINT | 2:
            case OP_CMD_DRAW_POINT | 3:
            case OP_CMD_DRAW_POINT | 4:
            case OP_CMD_DRAW_POINT | 5:
            case OP_CMD_DRAW_POINT | 6:
            case OP_CMD_DRAW_POINT | 7:
            {
                static  Vertex  p;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)readptr);
                usevat = vatnum;
                readptr += 2;
                                                            /*/
                    0---0       tri: 0-0-0 (1x1x1 tri)
                    |  /
                    | /
                    |/
                    0  
                                                            /*/

                while(vtxnum-- > 0)
                {
                    vtx = &p;
                    readptr = FifoWalk(vatnum, readptr);
                    gfx->RenderPoint(vtx);
                }
                break;
            }

            // ---------------------------------------------------------------
            // unknown fifo command

            default:
            {
                GFXError(
                    "Damaged FIFO buffer (in DL)! Some of attributes are not emulated :\n"
                    "\n"
                    "VCD configuration :\n"
                    "pmidx:%i\n"
                    "t0idx:%i\t tex0:%i\n"
                    "t1idx:%i\t tex1:%i\n"
                    "t2idx:%i\t tex2:%i\n"
                    "t3idx:%i\t tex3:%i\n"
                    "t4idx:%i\t tex4:%i\n"
                    "t5idx:%i\t tex5:%i\n"
                    "t6idx:%i\t tex6:%i\n"
                    "t7idx:%i\t tex7:%i\n"
                    "pos:%i\n"
                    "nrm:%i\n"
                    "col0:%i\n"
                    "col1:%i\n"
                    "\n"
                    "Additional information is saved to fifo.txt",
                    cpRegs.vcdLo.pmidx,
                    cpRegs.vcdLo.t0midx, cpRegs.vcdHi.tex0,
                    cpRegs.vcdLo.t1midx, cpRegs.vcdHi.tex1,
                    cpRegs.vcdLo.t2midx, cpRegs.vcdHi.tex2,
                    cpRegs.vcdLo.t3midx, cpRegs.vcdHi.tex3,
                    cpRegs.vcdLo.t4midx, cpRegs.vcdHi.tex4,
                    cpRegs.vcdLo.t5midx, cpRegs.vcdHi.tex5,
                    cpRegs.vcdLo.t6midx, cpRegs.vcdHi.tex6,
                    cpRegs.vcdLo.t7midx, cpRegs.vcdHi.tex7,
                    cpRegs.vcdLo.pos,
                    cpRegs.vcdLo.nrm,
                    cpRegs.vcdLo.col0,
                    cpRegs.vcdLo.col1
                );

                FILE *f = fopen("fifo.txt", "w");
                fprintf(f,
                    "Damaged FIFO buffer! Some of attributes are not emulated :\n"
                    "\n"
                    "VCD configuration :\n"
                    "\n"
                    "pmidx:%i\n"
                    "t0idx:%i\t tex0:%i\n"
                    "t1idx:%i\t tex1:%i\n"
                    "t2idx:%i\t tex2:%i\n"
                    "t3idx:%i\t tex3:%i\n"
                    "t4idx:%i\t tex4:%i\n"
                    "t5idx:%i\t tex5:%i\n"
                    "t6idx:%i\t tex6:%i\n"
                    "t7idx:%i\t tex7:%i\n"
                    "pos:%i\n"
                    "nrm:%i\n"
                    "col0:%i\n"
                    "col1:%i\n"
                    "\n",
                    cpRegs.vcdLo.pmidx,
                    cpRegs.vcdLo.t0midx, cpRegs.vcdHi.tex0,
                    cpRegs.vcdLo.t1midx, cpRegs.vcdHi.tex1,
                    cpRegs.vcdLo.t2midx, cpRegs.vcdHi.tex2,
                    cpRegs.vcdLo.t3midx, cpRegs.vcdHi.tex3,
                    cpRegs.vcdLo.t4midx, cpRegs.vcdHi.tex4,
                    cpRegs.vcdLo.t5midx, cpRegs.vcdHi.tex5,
                    cpRegs.vcdLo.t6midx, cpRegs.vcdHi.tex6,
                    cpRegs.vcdLo.t7midx, cpRegs.vcdHi.tex7,
                    cpRegs.vcdLo.pos,
                    cpRegs.vcdLo.nrm,
                    cpRegs.vcdLo.col0,
                    cpRegs.vcdLo.col1                    
                );
                fprintf(f, "Current VAT number : %i\n\n", usevat);
                for(unsigned v=0; v<8; v++)
                {
                    fprintf(f,
                        "VAT%i configuration :\n"
                        "\n"
                        "pos : (%i %i %i)\n"
                        "nrm : (%i %i)\n"
                        "col0: (%i %i)\n"
                        "col1: (%i %i)\n"
                        "tex0: (%i %i %i)\n"
                        "tex1: (%i %i %i)\n"
                        "tex2: (%i %i %i)\n"
                        "tex3: (%i %i %i)\n"
                        "tex4: (%i %i %i)\n"
                        "tex5: (%i %i %i)\n"
                        "tex6: (%i %i %i)\n"
                        "tex7: (%i %i %i)\n"
                        "\n",
                        v,
                        cpRegs.vatA[v].poscnt, cpRegs.vatA[v].posfmt, cpRegs.vatA[v].posshft,
                        cpRegs.vatA[v].nrmcnt, cpRegs.vatA[v].nrmfmt,
                        cpRegs.vatA[v].col0cnt, cpRegs.vatA[v].col0fmt,
                        cpRegs.vatA[v].col1cnt, cpRegs.vatA[v].col1fmt,
                        cpRegs.vatA[v].tex0cnt, cpRegs.vatA[v].tex0fmt, cpRegs.vatA[v].tex0shft,
                        cpRegs.vatB[v].tex1cnt, cpRegs.vatB[v].tex1fmt, cpRegs.vatB[v].tex1shft,
                        cpRegs.vatB[v].tex2cnt, cpRegs.vatB[v].tex2fmt, cpRegs.vatB[v].tex2shft,
                        cpRegs.vatB[v].tex3cnt, cpRegs.vatB[v].tex3fmt, cpRegs.vatB[v].tex3shft,
                        cpRegs.vatB[v].tex4cnt, cpRegs.vatB[v].tex4fmt, cpRegs.vatC[v].tex4shft,
                        cpRegs.vatC[v].tex5cnt, cpRegs.vatC[v].tex5fmt, cpRegs.vatC[v].tex5shft,
                        cpRegs.vatC[v].tex6cnt, cpRegs.vatC[v].tex6fmt, cpRegs.vatC[v].tex6shft,
                        cpRegs.vatC[v].tex7cnt, cpRegs.vatC[v].tex7fmt, cpRegs.vatC[v].tex7shft
                    );
                }
                fclose(f);
            }
        }
    }
}


static void gx_bad_fifo()
{
            {
                GFXError(
                    "Damaged FIFO buffer! Some of attributes are not emulated :\n"
                    "\n"
                    "VCD configuration :\n"
                    "pmidx:%i\n"
                    "t0idx:%i\t tex0:%i\n"
                    "t1idx:%i\t tex1:%i\n"
                    "t2idx:%i\t tex2:%i\n"
                    "t3idx:%i\t tex3:%i\n"
                    "t4idx:%i\t tex4:%i\n"
                    "t5idx:%i\t tex5:%i\n"
                    "t6idx:%i\t tex6:%i\n"
                    "t7idx:%i\t tex7:%i\n"
                    "pos:%i\n"
                    "nrm:%i\n"
                    "col0:%i\n"
                    "col1:%i\n"
                    "\n"
                    "Additional information is saved to fifo.txt",
                    cpRegs.vcdLo.pmidx,
                    cpRegs.vcdLo.t0midx, cpRegs.vcdHi.tex0,
                    cpRegs.vcdLo.t1midx, cpRegs.vcdHi.tex1,
                    cpRegs.vcdLo.t2midx, cpRegs.vcdHi.tex2,
                    cpRegs.vcdLo.t3midx, cpRegs.vcdHi.tex3,
                    cpRegs.vcdLo.t4midx, cpRegs.vcdHi.tex4,
                    cpRegs.vcdLo.t5midx, cpRegs.vcdHi.tex5,
                    cpRegs.vcdLo.t6midx, cpRegs.vcdHi.tex6,
                    cpRegs.vcdLo.t7midx, cpRegs.vcdHi.tex7,
                    cpRegs.vcdLo.pos,
                    cpRegs.vcdLo.nrm,
                    cpRegs.vcdLo.col0,
                    cpRegs.vcdLo.col1
                );

                FILE *f = fopen("fifo.txt", "w");
                fprintf(f,
                    "Damaged FIFO buffer! Some of attributes are not emulated :\n"
                    "\n"
                    "VCD configuration :\n"
                    "\n"
                    "pmidx:%i\n"
                    "t0idx:%i\t tex0:%i\n"
                    "t1idx:%i\t tex1:%i\n"
                    "t2idx:%i\t tex2:%i\n"
                    "t3idx:%i\t tex3:%i\n"
                    "t4idx:%i\t tex4:%i\n"
                    "t5idx:%i\t tex5:%i\n"
                    "t6idx:%i\t tex6:%i\n"
                    "t7idx:%i\t tex7:%i\n"
                    "pos:%i\n"
                    "nrm:%i\n"
                    "col0:%i\n"
                    "col1:%i\n"
                    "\n",
                    cpRegs.vcdLo.pmidx,
                    cpRegs.vcdLo.t0midx, cpRegs.vcdHi.tex0,
                    cpRegs.vcdLo.t1midx, cpRegs.vcdHi.tex1,
                    cpRegs.vcdLo.t2midx, cpRegs.vcdHi.tex2,
                    cpRegs.vcdLo.t3midx, cpRegs.vcdHi.tex3,
                    cpRegs.vcdLo.t4midx, cpRegs.vcdHi.tex4,
                    cpRegs.vcdLo.t5midx, cpRegs.vcdHi.tex5,
                    cpRegs.vcdLo.t6midx, cpRegs.vcdHi.tex6,
                    cpRegs.vcdLo.t7midx, cpRegs.vcdHi.tex7,
                    cpRegs.vcdLo.pos,
                    cpRegs.vcdLo.nrm,
                    cpRegs.vcdLo.col0,
                    cpRegs.vcdLo.col1                    
                );
                fprintf(f, "Current VAT number : %i\n\n", usevat);
                for(unsigned v=0; v<8; v++)
                {
                    fprintf(f,
                        "VAT%i configuration :\n"
                        "\n"
                        "pos : (%i %i %i)\n"
                        "nrm : (%i %i)\n"
                        "col0: (%i %i)\n"
                        "col1: (%i %i)\n"
                        "tex0: (%i %i %i)\n"
                        "tex1: (%i %i %i)\n"
                        "tex2: (%i %i %i)\n"
                        "tex3: (%i %i %i)\n"
                        "tex4: (%i %i %i)\n"
                        "tex5: (%i %i %i)\n"
                        "tex6: (%i %i %i)\n"
                        "tex7: (%i %i %i)\n"
                        "\n",
                        v,
                        cpRegs.vatA[v].poscnt, cpRegs.vatA[v].posfmt, cpRegs.vatA[v].posshft,
                        cpRegs.vatA[v].nrmcnt, cpRegs.vatA[v].nrmfmt,
                        cpRegs.vatA[v].col0cnt, cpRegs.vatA[v].col0fmt,
                        cpRegs.vatA[v].col1cnt, cpRegs.vatA[v].col1fmt,
                        cpRegs.vatA[v].tex0cnt, cpRegs.vatA[v].tex0fmt, cpRegs.vatA[v].tex0shft,
                        cpRegs.vatB[v].tex1cnt, cpRegs.vatB[v].tex1fmt, cpRegs.vatB[v].tex1shft,
                        cpRegs.vatB[v].tex2cnt, cpRegs.vatB[v].tex2fmt, cpRegs.vatB[v].tex2shft,
                        cpRegs.vatB[v].tex3cnt, cpRegs.vatB[v].tex3fmt, cpRegs.vatB[v].tex3shft,
                        cpRegs.vatB[v].tex4cnt, cpRegs.vatB[v].tex4fmt, cpRegs.vatC[v].tex4shft,
                        cpRegs.vatC[v].tex5cnt, cpRegs.vatC[v].tex5fmt, cpRegs.vatC[v].tex5shft,
                        cpRegs.vatC[v].tex6cnt, cpRegs.vatC[v].tex6fmt, cpRegs.vatC[v].tex6shft,
                        cpRegs.vatC[v].tex7cnt, cpRegs.vatC[v].tex7fmt, cpRegs.vatC[v].tex7shft
                    );
                }
                fclose(f);
            }
}

static void gx_command(u8 cmd)
{
    u32 *p32;

    if(frame_done)
    {
        lastFifoSize = 0;
        gfx->BeginFrame();
        frame_done = 0;
    }

        switch(cmd)
        {
            // do nothing
            case OP_CMD_NOP:
            case OP_CMD_INV:
                break;

            case OP_CMD_CALL_DL:
                p32 = (u32 *)accptr;

                GPCallList(
                    &RAM[swap32(p32[0]) & RAMMASK], 
                    swap32(p32[1])
                );

                accptr += 8;
                break;

            // ---------------------------------------------------------------
            // loading of internal regs
            
            case OP_CMD_LOAD_BPREG:
            {
                u32 word = swap32(*(u32 *)accptr);
                accptr += 4;
                loadBPReg(word >> 24, word & 0xffffff);
                break;
            }

            case OP_CMD_LOAD_CPREG:
            {
                u8 index = *accptr++;
                u32 word = swap32(*(u32 *)accptr);
                accptr += 4;
                loadCPReg(index, word);
                break;
            }

            case OP_CMD_LOAD_XFREG:
            {
                u16 reg, len, index;
                u32 *regData;

                len = swap16(*(u16 *)accptr) + 1;
                accptr += 2;
                index = swap16(*(u16 *)accptr);
                accptr += 2;

                // reverse bytes
                regData = (u32 *)accptr;
                for(reg=0; reg<len; reg++, accptr+=4)
                {
                    regData[reg] = swap32(regData[reg]);
                }

                loadXFRegs(index, len, regData);
                break;
            }

/*/
            case OP_CMD_LOAD_INDXA:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXB:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXC:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }

            case OP_CMD_LOAD_INDXD:
            {
                u16 idx, start, len;

                idx = swap16(*(u16 *)readptr);
                readptr += 2;
                start = swap16(*(u16 *)readptr);
                readptr += 2;
                len = (start >> 12) + 1;
                start &= 0xfff;

                break;
            }
/*/

            // ---------------------------------------------------------------
            // draw commands

            // 0x80
            case OP_CMD_DRAW_QUAD | 0:
            case OP_CMD_DRAW_QUAD | 1:
            case OP_CMD_DRAW_QUAD | 2:
            case OP_CMD_DRAW_QUAD | 3:
            case OP_CMD_DRAW_QUAD | 4:
            case OP_CMD_DRAW_QUAD | 5:
            case OP_CMD_DRAW_QUAD | 6:
            case OP_CMD_DRAW_QUAD | 7:
            {
                static   Vertex   quad[4];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                    1---2       tri1: 0-1-2
                    |  /|       tri2: 0-2-3
                    | / |
                    |/  |
                    0---3
                                                            /*/

                while(vtxnum > 0)
                {
                    for(unsigned n=0; n<4; n++)
                    {
                        vtx = &quad[n];
                        accptr = FifoWalk(vatnum, accptr);
                    }
                    gfx->RenderTriangle(&quad[0], &quad[1], &quad[2]);
                    gfx->RenderTriangle(&quad[0], &quad[2], &quad[3]);
                    vtxnum -= 4;
                }
                break;
            }

            // 0x90
            case OP_CMD_DRAW_TRIANGLE | 0:
            case OP_CMD_DRAW_TRIANGLE | 1:
            case OP_CMD_DRAW_TRIANGLE | 2:
            case OP_CMD_DRAW_TRIANGLE | 3:
            case OP_CMD_DRAW_TRIANGLE | 4:
            case OP_CMD_DRAW_TRIANGLE | 5:
            case OP_CMD_DRAW_TRIANGLE | 6:
            case OP_CMD_DRAW_TRIANGLE | 7:
            {
                static   Vertex   tri[3];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                    1---2       tri: 0-1-2
                    |  /
                    | /
                    |/
                    0  
                                                            /*/

                while(vtxnum > 0)
                {
                    for(unsigned n=0; n<3; n++)
                    {
                        vtx = &tri[n];
                        accptr = FifoWalk(vatnum, accptr);
                    }
                    gfx->RenderTriangle(&tri[0], &tri[1], &tri[2]);
                    vtxnum -= 3;
                }
                break;
            }

            // 0x98 - not sure !
            case OP_CMD_DRAW_STRIP | 0:
            case OP_CMD_DRAW_STRIP | 1:
            case OP_CMD_DRAW_STRIP | 2:
            case OP_CMD_DRAW_STRIP | 3:
            case OP_CMD_DRAW_STRIP | 4:
            case OP_CMD_DRAW_STRIP | 5:
            case OP_CMD_DRAW_STRIP | 6:
            case OP_CMD_DRAW_STRIP | 7:
            {
                static   Vertex   tri[3];
                unsigned c = 2, order[3] = { 0, 1, 2 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                        1---3---5   tri1: 0-1-2
                       /|  /|  /    tri2: 1-2-3
                      / | / | /     tri3: 2-3-4
                     /  |/  |/      tri4: 3-4-5
                    0---2---4       ...
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 3);

                vtx = &tri[0];
                accptr = FifoWalk(vatnum, accptr);
                vtx = &tri[1];
                accptr = FifoWalk(vatnum, accptr);
                vtxnum -= 2;

                while(vtxnum-- > 0)
                {
                    vtx = &tri[c++];
                    accptr = FifoWalk(vatnum, accptr);
                    if(c > 2) c = 0;

                    gfx->RenderTriangle(
                        &tri[order[0]],
                        &tri[order[1]], 
                        &tri[order[2]]
                    );

                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = order[2];
                    order[2] = tmp;
                }
                break;
            }

            // 0xA0
            case OP_CMD_DRAW_FAN | 0:
            case OP_CMD_DRAW_FAN | 1:
            case OP_CMD_DRAW_FAN | 2:
            case OP_CMD_DRAW_FAN | 3:
            case OP_CMD_DRAW_FAN | 4:
            case OP_CMD_DRAW_FAN | 5:
            case OP_CMD_DRAW_FAN | 6:
            case OP_CMD_DRAW_FAN | 7:
            {
                static   Vertex   tri[3];
                unsigned c = 2, order[2] = { 1, 2 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                    1---2---3   tri1: 0-1-2
                    |  /  _/    tri2: 0-2-3
                    | / _/      trin: 0-[n-1]-n
                    |/_/    
                    0/
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 3);

                vtx = &tri[0];
                accptr = FifoWalk(vatnum, accptr);
                vtx = &tri[1];
                accptr = FifoWalk(vatnum, accptr);
                vtxnum -= 2;

                while(vtxnum-- > 0)
                {
                    vtx = &tri[c];
                    accptr = FifoWalk(vatnum, accptr);
                    c = (c == 2) ? (c = 1) : (c = 2);

                    gfx->RenderTriangle(
                        &tri[0],
                        &tri[order[0]],
                        &tri[order[1]]
                    );

                    // order[0] <-> order[1]
                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = tmp;
                }
                break;
            }

            // 0xA8
            case OP_CMD_DRAW_LINE | 0:
            case OP_CMD_DRAW_LINE | 1:
            case OP_CMD_DRAW_LINE | 2:
            case OP_CMD_DRAW_LINE | 3:
            case OP_CMD_DRAW_LINE | 4:
            case OP_CMD_DRAW_LINE | 5:
            case OP_CMD_DRAW_LINE | 6:
            case OP_CMD_DRAW_LINE | 7:
            {
                static   Vertex   v[2];
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                        1   3   5
                       /   /   / 
                      /   /   / 
                     /   /   /      
                    0   2   4       
                                                            /*/
                if(vtxnum == 0) break;

                while(vtxnum > 0)
                {
                    vtx = &v[0];
                    accptr = FifoWalk(vatnum, accptr);
                    vtx = &v[1];
                    accptr = FifoWalk(vatnum, accptr);
                    gfx->RenderLine(&v[0], &v[1]);
                    vtxnum -= 2;
                }
                break;
            }

            // 0xB0
            case OP_CMD_DRAW_LINESTRIP | 0:
            case OP_CMD_DRAW_LINESTRIP | 1:
            case OP_CMD_DRAW_LINESTRIP | 2:
            case OP_CMD_DRAW_LINESTRIP | 3:
            case OP_CMD_DRAW_LINESTRIP | 4:
            case OP_CMD_DRAW_LINESTRIP | 5:
            case OP_CMD_DRAW_LINESTRIP | 6:
            case OP_CMD_DRAW_LINESTRIP | 7:
            {
                static   Vertex   v[2];
                unsigned c = 1, order[2] = { 0, 1 }, tmp;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                        1   3   5
                       /|  /|  / 
                      / | / | /  
                     /  |/  |/   
                    0   2   4    
                                                            /*/
                if(vtxnum == 0) break;
                ASSERT(vtxnum < 2);

                vtx = &v[0];
                accptr = FifoWalk(vatnum, accptr);
                vtxnum--;

                while(vtxnum-- > 0)
                {
                    vtx = &v[c++];
                    accptr = FifoWalk(vatnum, accptr);
                    if(c > 1) c = 0;

                    gfx->RenderLine(
                        &v[order[0]],
                        &v[order[1]]
                    );

                    tmp      = order[0];
                    order[0] = order[1];
                    order[1] = tmp;
                }
                break;
            }

            // 0xB8
            case OP_CMD_DRAW_POINT | 0:
            case OP_CMD_DRAW_POINT | 1:
            case OP_CMD_DRAW_POINT | 2:
            case OP_CMD_DRAW_POINT | 3:
            case OP_CMD_DRAW_POINT | 4:
            case OP_CMD_DRAW_POINT | 5:
            case OP_CMD_DRAW_POINT | 6:
            case OP_CMD_DRAW_POINT | 7:
            {
                static  Vertex  p;
                unsigned vatnum = cmd & 7;
                unsigned vtxnum = swap16(*(u16 *)accptr);
                usevat = vatnum;
                accptr += 2;
                                                            /*/
                    0---0       tri: 0-0-0 (1x1x1 tri)
                    |  /
                    | /
                    |/
                    0  
                                                            /*/

                while(vtxnum-- > 0)
                {
                    vtx = &p;
                    accptr = FifoWalk(vatnum, accptr);
                    gfx->RenderPoint(vtx);
                }
                break;
            }

            // ---------------------------------------------------------------
            // unknown fifo command
            
            default:
            {
                gx_bad_fifo();
            }
        }
}


void GXWriteFifo(u8 *dataPtr, u32 length)
{
    lastFifoSize += length;

    memcpy(&accptr[acclen], dataPtr, length);
    acclen += length;

    if(cmdidle)
    {
        if(acclen >= 3)
        {
            gxcmd = *accptr++; acclen--;
            need  = gx_needbytes(gxcmd);
            cmdidle = 0;
        }
    }

    if(acclen >= (need+2) && !cmdidle)
    {
        u8 *was = accptr;
        gx_command(gxcmd);
        acclen -= (u32)accptr - (u32)was;
        memcpy(accum, accptr, acclen);
        accptr = accum;
        cmdidle = 1;
    }
}
