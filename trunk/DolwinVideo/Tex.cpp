// texture manager
#include "GX.h"

#define MAX 32

//
// local and external data
//

static      TexEntry    tcache[MAX];
static      unsigned    tptr;
static      UINT        texlist[MAX+1];     // gl texture list (0 entry reserved)
static      u8          tlut[1024 * 1024];  // temporary TLUT buffer

TexEntry    *tID[8];                        // texture unit bindings

Color       rgbabuf[1024 * 1024];

// ---------------------------------------------------------------------------

void TexInit()
{
    memset(tcache, 0, sizeof(tcache));
    tptr = 1;

    glGenTextures(MAX, texlist);
    for(unsigned n=1; n<MAX; n++)
    {
        tcache[n].bind = n;
    }
}

void TexFree()
{
    for(unsigned n=0; n<MAX; n++)
    {
        if(tcache[n].rgbaData)
        {
            free(tcache[n].rgbaData);
            tcache[n].rgbaData = NULL;
        }
    }
}

// ---------------------------------------------------------------------------

static void DumpTexture(Color *rgbaBuf, u32 addr, int fmt, int width, int height)
{
    char    path[256];
    FILE    *f;
    u8      hdr[14 + 40];   // bmp header
    u16     *phdr;
    int     s, t;
    Color   *base = rgbaBuf;

    CreateDirectory(".\\TEX", NULL);
    sprintf(path, ".\\TEX\\tex_%08X_%i.bmp", addr, fmt);
    
    // create new file    
    f = fopen(path, "wb");

    // write hardcoded header
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M'; hdr[2] = 0x36;
    hdr[4] = 0x20; hdr[10] = 0x36;
    hdr[14] = 40; 
    phdr = (u16 *)(&hdr[0x12]); *phdr = (u16)width;
    phdr = (u16 *)(&hdr[0x16]); *phdr = (u16)height;
    hdr[26] = 1; hdr[28] = 24; hdr[36] = 0x20;
    fwrite(hdr, 1, sizeof(hdr), f);

    // write texture image
    // fuck microsoft with their flipped bitmaps
    for(s=0; s<height; s++)
    {
        for(t=0; t<width; t++)
        {
            rgbaBuf = &base[(height - s - 1) * width + t];
            u8  rgb[3];     // RGB triplet
            {
                Color c;
                c.RGBA = swap32(rgbaBuf->RGBA);
                rgb[0] = c.B;   // B
                rgb[1] = c.G;   // G
                rgb[2] = c.R;   // R
                rgbaBuf++;
                fwrite(rgb, 1, 3, f);
            }
        }
    }

    fclose(f);
}

static void GetTlutCol(Color *c, unsigned id, unsigned entry)
{
    u16  *tptr = (u16 *)(&tlut[(bpRegs.settlut[id].tmem << 9) + 2 * entry]);
    int fmt = bpRegs.settlut[id].fmt;

    switch(fmt)
    {
        case 0:     // IA8
        {
            c->A = *tptr & 0xff;
            c->R = c->G = c->B = *tptr >> 8;
        }
        return;

        case 1:     // RGB565
        {
            u16 p = swap16(*tptr);

            u8 r = p >> 11;
            u8 g = (p >> 5) & 0x3f;
            u8 b = p & 0x1f;

            c->R = (r << 3) | (r >> 2);
            c->G = (g << 2) | (g >> 4);
            c->B = (b << 3) | (b >> 2);
            c->A = 255;
        }
        return;

        case 2:     // RGB5A3
        {
            u16 p = swap16(*tptr);
            if(p >> 15)
            {
                p &= ~0x8000;   // clear A-bit

                u8 r = p >> 10;
                u8 g = (p >> 5) & 0x1f;
                u8 b = p & 0x1f;

                c->R = (r << 3) | (r >> 2);
                c->G = (g << 3) | (g >> 2);
                c->B = (b << 3) | (b >> 2);
                c->A = 255;
            }
            else
            {
                u8 r = (p >> 8) & 0xf;
                u8 g = (p >> 4) & 0xf;
                u8 b = p & 0xf;
                u8 a = p >> 12;

                c->R = (r << 4) | r;
                c->G = (g << 4) | g;
                c->B = (b << 4) | b;
                c->A = a | (a << 3) | ((a << 9) & 3);
            }
        }
        return;

        default:
        {
            GFXError("unknown tlut format : %i", fmt);
        }
    }
}

void RebindTexture(unsigned id)
{
    glBindTexture(GL_TEXTURE_2D, tID[id]->bind);

    // parameters
    // check for extension ?
#ifndef GL_MIRRORED_REPEAT_ARB
#define GL_MIRRORED_REPEAT_ARB          0x8370
#endif
    static u32 wrap[4] = { GL_CLAMP, GL_REPEAT, GL_MIRRORED_REPEAT_ARB, GL_REPEAT };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap[bpRegs.texmode0[id].wrap_s]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap[bpRegs.texmode0[id].wrap_t]);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, TEXMODE);

    static u32 filt[] = {
        GL_NEAREST,
        GL_LINEAR,
        GL_NEAREST_MIPMAP_NEAREST,
        GL_NEAREST_MIPMAP_LINEAR,
        GL_LINEAR_MIPMAP_NEAREST,
        GL_LINEAR_MIPMAP_LINEAR
    };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt[bpRegs.texmode0[id].min]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt[bpRegs.texmode0[id].mag]);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        tID[id]->dw, tID[id]->dh,
        0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        tID[id]->rgbaData
    );
}

void LoadTexture(u32 addr, int id, int fmt, int width, int height)
{
    BOOL doDump = FALSE;
    Color *texbuf;
    int oldw, oldh;
    int w, h;
    unsigned n;

    // check cache entries for coincidence
/*/
    for(n=1; n<MAX; n++)
    {
        if(fmt >= 8) break;
        if(
            (tcache[n].ramAddr == addr) &&
            (tcache[n].fmt == fmt     ) &&
            (tcache[n].w == width     ) &&
            (tcache[n].h == height    ) )
        {
            tID[id] = &tcache[n];
            return;
        }
    }
/*/
    n = 1;

    // free
/*/
    if(tcache[n].rgbaData)
    {
        free(tcache[n].rgbaData);
        tcache[n].rgbaData = NULL;
    }
/*/

    // new
    n = tptr;
    tcache[n].ramAddr = addr;
    tcache[n].rawData = &RAM[addr & RAMMASK];
    tcache[n].fmt = fmt;

    // aspect
    tcache[n].ds = tcache[n].dt = 1.0f;
    __asm       mov     eax, width
    __asm       bsr     eax, eax
    __asm       mov     w, eax
    if(width & ((1 << w) - 1)) w = 1 << (w+1);
    else w = width;
    tcache[n].ds = (float)width / (float)w;
    __asm       mov     eax, height
    __asm       bsr     eax, eax
    __asm       mov     h, eax
    if(height & ((1 << h) - 1)) h = 1 << (h+1);
    else h = height;
    tcache[n].dt = (float)height / (float)h;

    oldw = width;
    oldh = height;
    tcache[n].w = oldw;
    width = tcache[n].dw = w;
    tcache[n].h = oldh;
    height = tcache[n].dh = h;

    // allocate
/*/
    tcache[n].rgbaData = (Color *)malloc(width * height * 4);
    ASSERT(tcache[n].rgbaData == NULL);
/*/
    tcache[n].rgbaData = rgbabuf;
    texbuf = tcache[n].rgbaData;

    // convert texture
    switch(fmt)
    {
        // "intensity 4". 2 texels per byte, grayscale 0..15
        case TF_I4:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;

            // TODO : unroll
            for(t=0; t<oldh; t+=8)
            for(s=0; s<oldw; s+=8)
            for(v=0; v<8; v++)
            for(u=0; u<8; u+=2)
            {
                unsigned ofs = width * (t + v) + s + u;
                texbuf[ofs].R = 
                texbuf[ofs].G = 
                texbuf[ofs].B = 
                texbuf[ofs].A = (*ptr >> 4) | (*ptr & 0xf0);
                ofs++;
                texbuf[ofs].R = 
                texbuf[ofs].G = 
                texbuf[ofs].B = 
                texbuf[ofs].A = (*ptr << 4) | (*ptr & 0x0f);
                ptr++;
            }
            break;
        }

        // intensity 8-bit. R=G=B=A=I8.
        case TF_I8:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;

            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=8)
            for(v=0; v<4; v++)
            for(u=0; u<8; u++)
            {
                int ofs = width * (t + v) + s + u;
                texbuf[ofs].R = 
                texbuf[ofs].G = 
                texbuf[ofs].B = 
                texbuf[ofs].A = *ptr++;
            }
            break;
        }

        // intesity / alpha 4. one texels per byte
        case TF_IA4:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;

            // TODO : unroll
            for(s=0; s<(oldw/4); s++)  // tile hor
            for(t=0; t<(oldh/8); t++) // tile ver
            for(u=0; u<4; u++)  // texel hor
            for(v=0; v<8; v++)  // texel ver
            {
                int ofs = width * (u + 4 * s) + 8 * t + v;
                texbuf[ofs].R = 
                texbuf[ofs].G =
                texbuf[ofs].B = *ptr << 4;
                texbuf[ofs].A = *ptr >> 4;
                texbuf[ofs].RGBA = swap32(texbuf[ofs].RGBA);
                ptr++;
            }
            break;
        }

        case TF_IA8:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=4)
            for(v=0; v<4; v++)
            for(u=0; u<4; u++)
            {
                unsigned ofs = width * (t + v) + s + u;
                texbuf[ofs].A = *ptr++;
                texbuf[ofs].R = 
                texbuf[ofs].G =
                texbuf[ofs].B = *ptr++;
                texbuf[ofs].RGBA = swap32(texbuf[ofs].RGBA);
            }
            break;
        }

        case TF_RGB565:
        {
            int s, t, u, v;
            u16  *ptr = (u16 *)tcache[n].rawData;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=4)
            for(v=0; v<4; v++)
            for(u=0; u<4; u++)
            {
                unsigned ofs = width * (t + v) + s + u;
                u16 p = swap16(*ptr++);

                u8 r = p >> 11;
                u8 g = (p >> 5) & 0x3f;
                u8 b = p & 0x1f;

                texbuf[ofs].R = (r << 3) | (r >> 2);
                texbuf[ofs].G = (g << 2) | (g >> 4);
                texbuf[ofs].B = (b << 3) | (b >> 2);
                texbuf[ofs].A = 255;
                texbuf[ofs].RGBA = swap32(texbuf[ofs].RGBA);
            }
            break;
        }

        case TF_RGB5A3:
        {
            int s, t, u, v;
            u16  *ptr = (u16 *)tcache[n].rawData;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=4)
            for(v=0; v<4; v++)
            for(u=0; u<4; u++)
            {
                unsigned ofs = width * (t + v) + s + u;
                u16 p = swap16(*ptr++);
                if(p >> 15)
                {
                    p &= ~0x8000;   // clear A-bit

                    u8 r = p >> 10;
                    u8 g = (p >> 5) & 0x1f;
                    u8 b = p & 0x1f;

                    texbuf[ofs].R = (r << 3) | (r >> 2);
                    texbuf[ofs].G = (g << 3) | (g >> 2);
                    texbuf[ofs].B = (b << 3) | (b >> 2);
                    texbuf[ofs].A = 255;
                }
                else
                {
                    u8 r = (p >> 8) & 0xf;
                    u8 g = (p >> 4) & 0xf;
                    u8 b = p & 0xf;
                    u8 a = p >> 12;

                    texbuf[ofs].R = (r << 4) | r;
                    texbuf[ofs].G = (g << 4) | g;
                    texbuf[ofs].B = (b << 4) | b;
                    texbuf[ofs].A = a | (a << 3) | ((a << 9) & 3);
                }
                texbuf[ofs].RGBA = swap32(texbuf[ofs].RGBA);
            }
            break;
        }

        case TF_RGBA8:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=4)
            {
                for(v=0; v<4; v++)
                for(u=0; u<4; u++)
                {
                    unsigned ofs = width * (t + v) + s + u;
                    texbuf[ofs].A = *ptr++;
                    texbuf[ofs].R = *ptr++;
                }

                for(v=0; v<4; v++)
                for(u=0; u<4; u++)
                {
                    unsigned ofs = width * (t + v) + s + u;
                    texbuf[ofs].G = *ptr++;
                    texbuf[ofs].B = *ptr++;
                    texbuf[ofs].RGBA = swap32(texbuf[ofs].RGBA);
                }
            }
            break;
        }

        case TF_C4:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;
            Color rgba;

            // TODO : unroll
            for(t=0; t<oldh; t+=8)
            for(s=0; s<oldw; s+=8)
            for(v=0; v<8; v++)
            for(u=0; u<8; u+=2)
            {
                unsigned ofs = width * (t + v) + s + u;
                u8 e = *ptr++;
                GetTlutCol(&rgba, id, e >> 4);
                texbuf[ofs].RGBA = swap32(rgba.RGBA);
                ofs++;
                GetTlutCol(&rgba, id, e & 0xf);
                texbuf[ofs].RGBA = swap32(rgba.RGBA);
            }
            break;
        }

        case TF_C8:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;
            Color rgba;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=8)
            for(v=0; v<4; v++)
            for(u=0; u<8; u++)
            {
                int ofs = width * (t + v) + s + u;
                u8 idx = *ptr++;
                GetTlutCol(&rgba, id, idx);
                texbuf[ofs].RGBA = swap32(rgba.RGBA);
            }
            break;
        }

        case TF_C14:
        {
            int s, t, u, v;
            u16  *ptr = (u16 *)tcache[n].rawData;
            Color rgba;

            // TODO : unroll
            for(t=0; t<oldh; t+=4)
            for(s=0; s<oldw; s+=4)
            for(v=0; v<4; v++)
            for(u=0; u<4; u++)
            {
                unsigned ofs = width * (t + v) + s + u;
                u16 idx = swap16(*ptr++) & 0x3ff;
                GetTlutCol(&rgba, id, idx);
                texbuf[ofs].RGBA = swap32(rgba.RGBA);
            }
            break;
        };

        case TF_CMPR:
        {
            int s, t, u, v;
            u8  *ptr = tcache[n].rawData;
            Color rgb[4];   // color look-up
            u8 r, g, b;
            u8 tnum;
            u16 p;
            S3TC_BLK blk;

            // 4 blocks by 4x4 texels in a row
            // zigzag order
            for(t=0; t<oldh; t+=8)
            for(s=0; s<oldw; s+=8)
            {
                tnum = 0;
                memcpy(&blk, ptr, sizeof(S3TC_BLK));
                ptr += sizeof(S3TC_BLK);
                for(v=0; v<4; v++)
                {
                    p = swap16(blk.rgb0);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[0].R = (r << 3) | (r >> 2);
                    rgb[0].G = (g << 2) | (g >> 4);
                    rgb[0].B = (b << 3) | (b >> 2);

                    p = swap16(blk.rgb1);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[1].R = (r << 3) | (r >> 2);
                    rgb[1].G = (g << 2) | (g >> 4);
                    rgb[1].B = (b << 3) | (b >> 2);
                    
                    // interpolate two other
                    if(blk.rgb0 > blk.rgb1)
                    {
                        rgb[2].R = (2 * rgb[0].R + rgb[1].R) / 3;
                        rgb[2].G = (2 * rgb[0].G + rgb[1].G) / 3;
                        rgb[2].B = (2 * rgb[0].B + rgb[1].B) / 3;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = 255;
                    }
                    else
                    {
                        rgb[2].R = (rgb[0].R + rgb[1].R) / 2;
                        rgb[2].G = (rgb[0].G + rgb[1].G) / 2;
                        rgb[2].B = (rgb[0].B + rgb[1].B) / 2;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = (2 * rgb[1].A + rgb[0].A) / 3;
                    }

                    u8 texel = blk.row[tnum++];
                    int shft;
                    for(u=0, shft=6; u<4; u++, shft-=2)
                    {
                        unsigned ofs = width * (t + v) + s + u;
                        u8 p = (texel >> shft) & 3;
                        texbuf[ofs].RGBA = swap32(rgb[p].RGBA);
                    }
                }

                tnum = 0;
                memcpy(&blk, ptr, sizeof(S3TC_BLK));
                ptr += sizeof(S3TC_BLK);
                for(v=0; v<4; v++)
                {
                    p = swap16(blk.rgb0);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[0].R = (r << 3) | (r >> 2);
                    rgb[0].G = (g << 2) | (g >> 4);
                    rgb[0].B = (b << 3) | (b >> 2);

                    p = swap16(blk.rgb1);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[1].R = (r << 3) | (r >> 2);
                    rgb[1].G = (g << 2) | (g >> 4);
                    rgb[1].B = (b << 3) | (b >> 2);
                    
                    // interpolate two other
                    if(blk.rgb0 > blk.rgb1)
                    {
                        rgb[2].R = (2 * rgb[0].R + rgb[1].R) / 3;
                        rgb[2].G = (2 * rgb[0].G + rgb[1].G) / 3;
                        rgb[2].B = (2 * rgb[0].B + rgb[1].B) / 3;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = 255;
                    }
                    else
                    {
                        rgb[2].R = (rgb[0].R + rgb[1].R) / 2;
                        rgb[2].G = (rgb[0].G + rgb[1].G) / 2;
                        rgb[2].B = (rgb[0].B + rgb[1].B) / 2;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = (2 * rgb[1].A + rgb[0].A) / 3;
                    }
                    
                    u8 texel = blk.row[tnum++];
                    int shft;
                    for(u=4, shft=6; u<8; u++, shft-=2)
                    {
                        unsigned ofs = width * (t + v) + s + u;
                        u8 p = (texel >> shft) & 3;
                        texbuf[ofs].RGBA = swap32(rgb[p].RGBA);
                    }
                }

                tnum = 0;
                memcpy(&blk, ptr, sizeof(S3TC_BLK));
                ptr += sizeof(S3TC_BLK);
                for(v=4; v<8; v++)
                {
                    p = swap16(blk.rgb0);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[0].R = (r << 3) | (r >> 2);
                    rgb[0].G = (g << 2) | (g >> 4);
                    rgb[0].B = (b << 3) | (b >> 2);

                    p = swap16(blk.rgb1);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[1].R = (r << 3) | (r >> 2);
                    rgb[1].G = (g << 2) | (g >> 4);
                    rgb[1].B = (b << 3) | (b >> 2);
                    
                    // interpolate two other
                    if(blk.rgb0 > blk.rgb1)
                    {
                        rgb[2].R = (2 * rgb[0].R + rgb[1].R) / 3;
                        rgb[2].G = (2 * rgb[0].G + rgb[1].G) / 3;
                        rgb[2].B = (2 * rgb[0].B + rgb[1].B) / 3;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = 255;
                    }
                    else
                    {
                        rgb[2].R = (rgb[0].R + rgb[1].R) / 2;
                        rgb[2].G = (rgb[0].G + rgb[1].G) / 2;
                        rgb[2].B = (rgb[0].B + rgb[1].B) / 2;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = (2 * rgb[1].A + rgb[0].A) / 3;
                    }

                    u8 texel = blk.row[tnum++];
                    int shft;
                    for(u=0, shft=6; u<4; u++, shft-=2)
                    {
                        unsigned ofs = width * (t + v) + s + u;
                        u8 p = (texel >> shft) & 3;
                        texbuf[ofs].RGBA = swap32(rgb[p].RGBA);
                    }
                }

                tnum = 0;
                memcpy(&blk, ptr, sizeof(S3TC_BLK));
                ptr += sizeof(S3TC_BLK);
                for(v=4; v<8; v++)
                {
                    p = swap16(blk.rgb0);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[0].R = (r << 3) | (r >> 2);
                    rgb[0].G = (g << 2) | (g >> 4);
                    rgb[0].B = (b << 3) | (b >> 2);
                    rgb[0].A = 255;

                    p = swap16(blk.rgb1);
                    r = p >> 11;
                    g = (p >> 5) & 0x3f;
                    b = p & 0x1f;

                    rgb[1].R = (r << 3) | (r >> 2);
                    rgb[1].G = (g << 2) | (g >> 4);
                    rgb[1].B = (b << 3) | (b >> 2);
                    rgb[1].A = 255;
                    
                    // interpolate two other
                    if(blk.rgb0 > blk.rgb1)
                    {
                        rgb[2].R = (2 * rgb[0].R + rgb[1].R) / 3;
                        rgb[2].G = (2 * rgb[0].G + rgb[1].G) / 3;
                        rgb[2].B = (2 * rgb[0].B + rgb[1].B) / 3;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = 255;
                    }
                    else
                    {
                        rgb[2].R = (rgb[0].R + rgb[1].R) / 2;
                        rgb[2].G = (rgb[0].G + rgb[1].G) / 2;
                        rgb[2].B = (rgb[0].B + rgb[1].B) / 2;
                        rgb[2].A = 255;
                        rgb[3].R = (2 * rgb[1].R + rgb[0].R) / 3;
                        rgb[3].G = (2 * rgb[1].G + rgb[0].G) / 3;
                        rgb[3].B = (2 * rgb[1].B + rgb[0].B) / 3;
                        rgb[3].A = (2 * rgb[1].A + rgb[0].A) / 3;
                    }

                    u8 texel = blk.row[tnum++];
                    int shft;
                    for(u=4, shft=6; u<8; u++, shft-=2)
                    {
                        unsigned ofs = width * (t + v) + s + u;
                        u8 p = (texel >> shft) & 3;
                        texbuf[ofs].RGBA = swap32(rgb[p].RGBA);
                    }
                }
            }
            break;
        };

        //default:
            //GFXError("Unknown texture format : %i\n", fmt);
    }

    // dump
    if(doDump)
    {
        DumpTexture(
            texbuf,
            addr,
            fmt,
            width,
            height
        );
    }

    // save
    tID[id] = &tcache[n];
/*/
    tptr++;
    if(tptr >= MAX)
    {
        tptr = 1;
        tcache[tptr].ramAddr = 0;
    }
/*/

    RebindTexture(id);
}

void LoadTlut(u32 addr, u32 tmem, u32 cnt)
{
    ASSERT(tmem >= sizeof(tlut));
    memcpy(&tlut[tmem], &RAM[addr], cnt * 16 * 2);
}
