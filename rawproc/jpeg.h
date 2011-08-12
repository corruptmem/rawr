#ifndef rawproc_jpeg_h
#define rawproc_jpeg_h
/*
typedef  unsigned short ushort;
typedef unsigned char uchar;
typedef long long INT64;
typedef unsigned long long UINT64;*/

struct jhead {
    int bits, high, wide, clrs, sraw, psv, restart, vpred[6];
    unsigned short *huff[6], *free[4], *row;
};

class ljpeg {
private: 
    bool _is_initialized;
    struct jhead* jh;
    
    unsigned int _bitbuf;
    int _vbits;
    
    std::istream& _data;
    
    void Init();
    unsigned short* MakeDecoderRef(const unsigned char **source);
    unsigned getbithuff (int nbits, unsigned short *huff);
    unsigned getbits(int nbits);
    int diff (unsigned short *huff);
    
    template<typename T> unsigned gethuff(T& h) {
        return getbithuff(*h, h+1);
    }
    
public:
    struct jhead jhactual;
    
    ljpeg(std::istream& data);


    int get_height();
    int get_width();

    
    void end ();

    unsigned short * row (int jrow);
    
};
#endif
