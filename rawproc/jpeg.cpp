#include <string.h>
#include <iostream>

#include "jpeg.h"

ljpeg::ljpeg(std::istream& data) : SeekCur(1), _data(data) {
    jh = &jhactual;
    _pos = 0;
}

unsigned ljpeg::getbithuff (int nbits, ushort *huff)
{
    static unsigned bitbuf=0;
    static int vbits=0, reset=0;
    unsigned c;
    
    if (nbits == -1)
    {
        bitbuf = 0;
        vbits = 0;
        reset = 0;
        
        return 0;
    }
    
    if (nbits == 0 || vbits < 0) {
        return 0;
    }
    
    while(true) {
        if(vbits >= nbits) break;
        
        if(_data.eof()) break;
        
        c = _data.get();
    
        
        if(zero_after_ff && c == 0xff) {
            if(_data.get()) {
                reset = 1;
                break;
            }
        }
        
        reset = 0;
        
        bitbuf = (bitbuf << 8) + (uchar) c;
        vbits += 8;
    }
    
    c = bitbuf << (32-vbits) >> (32-nbits);
    if (huff) {
        vbits -= huff[c] >> 8;
        c = (uchar) huff[c];
    } else {
        vbits -= nbits;
    }
    
    if (vbits < 0) {
        throw "Vbits was 0";
    }
    
    return c;
}

unsigned ljpeg::getbits(int nbits) {
    return getbithuff(nbits, 0);
}

ushort * ljpeg::make_decoder_ref (const uchar **source)
{
    int max, len, h, i, j;
    const uchar *count;
    ushort *huff;
    
    *source += 16;
    count = *source - 17;
    
    for (max=16; max && !count[max]; max--);
    
    uint32_t huff_size = 1+(1<<max);
    huff = new ushort[huff_size];
    memset(huff, 0, huff_size);
    
    huff[0] = max;
    for (h=len=1; len <= max; len++) {
        for (i=0; i < count[len]; i++, ++*source) {
            for (j=0; j < 1 << (max-len); j++) {
                if (h <= 1 << max) {
                    huff[h++] = len << 8 | **source;
                }
            }
        }
    }
    return huff;
}

int ljpeg::start()
{
    int c, tag, len;
    uchar data[0x10000];
    const uchar *dp;
            
    memset (jh, 0, sizeof *jh);
    jh->restart = INT_MAX;
    _data.read((char*)data, 2);
    
    if (data[1] != 0xd8) {
        return 0;
    }
    
    do {
        _data.read((char*)data, 4);
        
        tag =  data[0] << 8 | data[1];
        len = (data[2] << 8 | data[3]) - 2;
        
        if (tag <= 0xff00) {
            return 0;
        }
        
        _data.read((char*)data, len);
        
        switch (tag) {
            case 0xffc3:
                jh->sraw = ((data[7] >> 4) * (data[7] & 15) - 1) & 3;
                
            case 0xffc0:
                jh->bits = data[0];
                jh->high = data[1] << 8 | data[2];
                jh->wide = data[3] << 8 | data[4];
                jh->clrs = data[5] + jh->sraw;
                
                if (len == 9 && !dng_version) {
                    _data.get();
                }
                
                break;
                
            case 0xffc4:
                for (dp = data; dp < data+len && (c = *dp++) < 4; ) {
                    jh->free[c] = jh->huff[c] = make_decoder_ref (&dp);
                }
                
                break;
                
            case 0xffda:
                jh->psv = data[1+data[0]*2];
                jh->bits -= data[3+data[0]*2] & 15;
                break;
                
            case 0xffdd:
                jh->restart = data[0] << 8 | data[1];
        }
    } while (tag != 0xffda);
    
    for(int c = 0; c < 5; c++) {
        if(!jh->huff[c+1]) {
            jh->huff[c+1] = jh->huff[c];
        }
    }
    
    if (jh->sraw) {
        for(int c = 0; c < 4; c++) {
            jh->huff[2+c] = jh->huff[1];
        }
        
        for(int c = 0; c<jh->sraw; c++) {
            jh->huff[1+c] = jh->huff[0];
        }
    }
    
    jh->row = (ushort *) calloc (jh->wide*jh->clrs, 4);
    merror (jh->row, "ljpeg_start()");
    return zero_after_ff = 1;
}

void ljpeg::merror (void *ptr, const char *where)
{
    if (ptr) return;
    throw "Out of memory";
}

void ljpeg::end ()
{
    for(int c = 0; c<4; c++) {
        if (jh->free[c]) {
            free (jh->free[c]);
        }
    }
    
    free (jh->row);
}

int ljpeg::diff (ushort *huff)
{
    int len, diff;
    
    len = gethuff(huff);
    
    if (len == 16 && (!dng_version || dng_version >= 0x1010000)) {
        return -32768;
    }
    
    diff = getbits(len);
    
    if ((diff & (1 << (len-1))) == 0) {
        diff -= (1 << len) - 1;
    }
    
    return diff;
}

ushort * ljpeg::row (int jrow)
{
    int diff, pred, spred=0;
    ushort mark=0, *row[3];

    if (jrow * jh->wide % jh->restart == 0) {
        for(int c = 0; c<6; c++) {
            jh->vpred[c] = 1 << (jh->bits-1);
        }
        
        if (jrow) {
            int c;
            _data.seekg (-2, std::ios_base::beg);
            _data >> c;
            
            do {
                mark = (mark << 8) + c;
            }
            while (mark >> 4 != 0xffd);
        }
        getbits(-1);
    }
    
    for(int c = 0; c<3; c++) {
        row[c] = jh->row + jh->wide*jh->clrs*((jrow+c) & 1);
    }
    
    for (int col=0; col < jh->wide; col++)
        for(int c = 0; c<jh->clrs; c++) {
            diff = this->diff (jh->huff[c]);
            if (jh->sraw && c <= jh->sraw && (col | c)) {
                pred = spred;
            } else if (col) {
                pred = row[0][-jh->clrs];
            } else{
                pred = (jh->vpred[c] += diff) - diff;
            }
            
            if (jrow && col) {
                switch (jh->psv) {
                    case 1:	
                        break;
                        
                    case 2: 
                        pred = row[1][0];					
                        break;
                        
                    case 3: 
                        pred = row[1][-jh->clrs];
                        break;
                        
                    case 4: 
                        pred = pred + row[1][0] - row[1][-jh->clrs];
                        break;
                        
                    case 5: 
                        pred = pred + ((row[1][0] - row[1][-jh->clrs]) >> 1);
                        break;
                        
                    case 6:
                        pred = row[1][0] + ((pred - row[1][-jh->clrs]) >> 1);
                        break;
                        
                    case 7: 
                        pred = (pred + row[1][0]) >> 1;
                        break;
                        
                    default: 
                        pred = 0;
                }
            }
            
            if ((**row = pred + diff) >> jh->bits) {
                throw "Data error";
            }
            
            if (c <= jh->sraw) {
                spred = **row;
            }
            
            row[0]++; 
            row[1]++;
        }
                
    return row[2];
}
