#include <string.h>
#include <iostream>

#include "jpeg.h"

ljpeg::ljpeg(std::istream& data) : 
        _data(data), 
        _bitbuf(0), 
        _vbits(0),
        _is_initialized(0),
        _bits(0),
        _high(0),
        _wide(0),
        _clrs(0),
        _psv(0),
        _vpred(),
        _huff(),
        _free(),
        _cur_row(0),
        _row()
{
}

unsigned int ljpeg::getbithuff (int nbits, unsigned short *huff)
{
    if (nbits == -1)
    {
        _bitbuf = 0;
        _vbits = 0;
        
        return 0;
    }
    
    if (nbits == 0 || _vbits < 0) {
        return 0;
    }
    
    while(true) {
        if(_vbits >= nbits) break;
        if(_data.eof()) break;
        
        unsigned int c = _data.get();
    
        if(c == 0xff) {
            if(_data.get()) {
                break;
            }
        }
        
        _bitbuf = (_bitbuf << 8) + (unsigned char) c;
        _vbits += 8;
    }
    
    unsigned int c = _bitbuf << (32-_vbits) >> (32-nbits);
    if (huff) {
        _vbits -= huff[c] >> 8;
        c = (unsigned char) huff[c];
    } else {
        _vbits -= nbits;
    }
    
    if (_vbits < 0) {
        throw "Vbits was 0";
    }
    
    return c;
}

unsigned int ljpeg::getbits(int nbits) {
    return getbithuff(nbits, 0);
}

unsigned short* ljpeg::MakeDecoderRef(const unsigned char **source)
{
    int max, len, h, i, j;
    const unsigned char *count;
    unsigned short *huff;
    
    *source += 16;
    count = *source - 17;
    
    for (max=16; max && !count[max]; max--);
    
    uint32_t huff_size = 1+(1<<max);
    huff = new unsigned short[huff_size];
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

void ljpeg::Init()
{
    if(_is_initialized) return;
    _is_initialized = true;
    
    unsigned char ljpeg_header[2];
    
    _data.read((char*)ljpeg_header, 2);
    
    if (ljpeg_header[1] != 0xd8) {
        throw "Didn't understand LJPEG header (1)";
    }
    
    int tag;
    do {
        unsigned char tag_header[4];
        _data.read((char*)tag_header, 4);
        
        tag = tag_header[0] << 8 | tag_header[1];
        int len = (tag_header[2] << 8 | tag_header[3]) - 2;
        
        if (tag <= 0xff00) {
            throw "Didn't understand LJPEG header (2)";
        }
        
        unsigned char tag_data[len];
        _data.read((char*)tag_data, len);
        
        switch (tag) {
            case 0xffc3:
            case 0xffc0:
                _bits = tag_data[0];
                _high = tag_data[1] << 8 | tag_data[2];
                _wide = tag_data[3] << 8 | tag_data[4];
                _clrs = tag_data[5];
                
                if (len == 9) {
                    _data.get();
                }
                
                break;
                
            case 0xffc4:
                int c;
                for (const unsigned char* dp = tag_data; dp < tag_data+len && (c = *dp++) < 4; ) {
                    _free[c] = _huff[c] = MakeDecoderRef(&dp);
                }
                
                break;
                
            case 0xffda:
                _psv = tag_data[1+tag_data[0]*2];
                _bits -= tag_data[3+tag_data[0]*2] & 15;
                break;
        }
    } while (tag != 0xffda);
    
    for(int c = 0; c < 5; c++) {
        if(!_huff[c+1]) {
            _huff[c+1] = _huff[c];
        }
    }
    
    _row = (unsigned short*) calloc (_wide*_clrs, 4);
}

ljpeg::~ljpeg()
{
    for(int c = 0; c<4; c++) {
        if (_free[c]) {
            free (_free[c]);
        }
    }
    
    free (_row);
}

int ljpeg::diff (unsigned short *huff)
{
    int len, diff;
    
    len = gethuff(huff);
    
    if (len == 16) {
        throw "len was 16";
    }
    
    diff = getbits(len);
    
    if ((diff & (1 << (len-1))) == 0) {
        diff -= (1 << len) - 1;
    }
    
    return diff;
}

int ljpeg::get_height() {
    Init();
    return _high;
}

int ljpeg::get_width() {
    Init();
    return _wide;
}

unsigned short * ljpeg::row()
{
    Init();
    
    if(_cur_row > get_height()) {
        throw "Tried to get row past EOF";
    }
    
    int diff, pred, spred=0;
    unsigned short *row[3];

    if (_cur_row == 0) {
        for(int c = 0; c<6; c++) {
            _vpred[c] = 1 << (_bits-1);
        }
        
        getbits(-1);
    }
    
    for(int c = 0; c<3; c++) {
        row[c] = _row + _wide*_clrs*((_cur_row+c) & 1);
    }
    
    for (int col=0; col < _wide; col++)
        for(int c = 0; c<_clrs; c++) {
            diff = this->diff (_huff[c]);
            if (col) {
                pred = row[0][-_clrs];
            } else{
                pred = (_vpred[c] += diff) - diff;
            }
            
            if (_cur_row && col) {
                switch (_psv) {
                    case 1:	
                        break;
                        
                    case 2: 
                        pred = row[1][0];					
                        break;
                        
                    case 3: 
                        pred = row[1][-_clrs];
                        break;
                        
                    case 4: 
                        pred = pred + row[1][0] - row[1][-_clrs];
                        break;
                        
                    case 5: 
                        pred = pred + ((row[1][0] - row[1][-_clrs]) >> 1);
                        break;
                        
                    case 6:
                        pred = row[1][0] + ((pred - row[1][-_clrs]) >> 1);
                        break;
                        
                    case 7: 
                        pred = (pred + row[1][0]) >> 1;
                        break;
                        
                    default: 
                        pred = 0;
                }
            }
            
            if ((**row = pred + diff) >> _bits) {
                throw "Data error";
            }

            spred = **row;
            
            row[0]++; 
            row[1]++;
        }
    
    _cur_row++;
    return row[2];
}
