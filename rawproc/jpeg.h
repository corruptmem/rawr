//
//  jpeg.h
//  rawproc
//
//  Created by Cameron Harris on 02/08/2011.
//  Copyright 2011 Altis Partners. All rights reserved.
//

#ifndef rawproc_jpeg_h
#define rawproc_jpeg_h

typedef  unsigned short ushort;
typedef unsigned char uchar;
typedef long long INT64;
typedef unsigned long long UINT64;

struct jhead {
    int bits, high, wide, clrs, sraw, psv, restart, vpred[6];
    ushort *huff[6], *free[4], *row;
};

class ljpeg {
private: 
    unsigned data_error;
    const char* ifname;
    unsigned zero_after_ff;
    unsigned dng_version;
    struct jhead* jh;
    struct jhead jhactual;
    const char* _data;
    int _size;
    int _pos;
public:
    ljpeg(const char* data, int size);
    void fread(unsigned char* data, int m, int n);
    unsigned int fgetc();
    unsigned int getc();
    void fseek(int amount, int seektype);
    unsigned  getbithuff (int nbits, ushort *huff);
    unsigned getbits(int nbits);
    
    template<typename T> unsigned gethuff(T& h) {
        return getbithuff(*h, h+1);
    }
    
    ushort * make_decoder_ref (const uchar **source);
    int start (int info_only);
    void merror (void *ptr, const char *where);
    void end ();
    int diff (ushort *huff);
    ushort * row (int jrow);
    
};
#endif
