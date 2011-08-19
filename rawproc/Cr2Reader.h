#include <fstream>

#include "LJpeg.h"

#ifndef rawproc_Cr2Reader_h
#define rawproc_Cr2Reader_h

struct RawSensel {
    double val;
    double prob;
};

class Cr2Reader {
private:
    const char* _path;
    int _sensor_width;
    int _sensor_height;
    int _sensor_left_border;
    int _sensor_right_border;
    int _sensor_bottom_border;
    int _sensor_top_border;
    
    int _first_strip_count;
    int _first_strip_px;
    int _last_strip_px;
    
    int _raw_ptr;
    
    double _sensitivity;
    double _exposure_time;
    double _f_number;
    
    double _min_val;
    double _max_val;
    
    bool _file_parsed;
    
    double _base_probs[65536];
    
    double GetBaseProb(unsigned short px);
    
    std::ifstream _file;

public:
    Cr2Reader(const char* path);
    
    int get_width();
    int get_height();
    
    void Parse();
    RawSensel* Process();
};

#endif
