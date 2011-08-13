#ifndef rawproc_jpeg_h
#define rawproc_jpeg_h
/*
typedef  unsigned short ushort;
typedef unsigned char uchar;
typedef long long INT64;
typedef unsigned long long UINT64;*/


class ljpeg {
private: 
    bool _is_initialized;
    std::istream& _data;
    
    
    unsigned int _bitbuf;
    int _vbits;
    
    /* jhead shit */
    
    int _bits, _high, _wide, _clrs, _sraw, _psv, _restart, _vpred[6];
    unsigned short *_huff[6], *_free[4], *_row;
    
    /* -- */
    
    void Init();
    
    unsigned short* MakeDecoderRef(const unsigned char **source);
    unsigned int getbithuff (int nbits, unsigned short *huff);
    unsigned int getbits(int nbits);
    int diff (unsigned short *huff);
    
    template<typename T> unsigned int gethuff(T& h) {
        return getbithuff(*h, h+1);
    }
    
public:
    ljpeg(std::istream& data);
    ~ljpeg();


    int get_height();
    int get_width();

    unsigned short * row (int jrow);
    
};
#endif
