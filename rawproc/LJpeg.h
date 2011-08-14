#include <fstream>

#ifndef rawproc_LJpeg_h
#define rawproc_LJpeg_h

class LJpeg {
private: 
    bool _is_initialized;
    std::istream& _data;
    
    unsigned int _bitbuf;
    int _vbits;
    int _cur_row;
    
    int _bits, _high, _wide, _clrs, _psv, _vpred[6];
    unsigned short *_huff[6], *_free[4], *_row;

    void Init();
    
    unsigned short* MakeDecoderRef(const unsigned char **source);
    unsigned int getbithuff (int nbits, unsigned short *huff);
    unsigned int getbits(int nbits);
    int diff (unsigned short *huff);
    
    template<typename T> unsigned int gethuff(T& h) {
        return getbithuff(*h, h+1);
    }
    
public:
    LJpeg(std::istream& data);
    ~LJpeg();

    int get_height();
    int get_width();

    unsigned short* row();
};

#endif
