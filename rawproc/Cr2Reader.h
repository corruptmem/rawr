#include <fstream>

#include "jpeg.h"

#ifndef rawproc_Cr2Reader_h
#define rawproc_Cr2Reader_h

struct Cr2Header {
    uint16_t byte_order;
    uint16_t magic_word;
    uint32_t tiff_offset;
    uint16_t cr2_magic_word;
    uint8_t cr2_major_ver;
    uint8_t cr2_minor_ver;
    uint32_t raw_ifd_offset;
    
public:
    bool is_valid_header();
};

struct Cr2IfdEntry {
    uint16_t tag_id;
    uint16_t tag_type;
    uint32_t number_of_val;
    uint32_t val;
};

struct Cr2IfdHeader {
    uint16_t entries_count;
};

struct Cr2IfdFooter {
    uint32_t next_ifd;
};

struct Cr2Slices {
    uint16_t num_first_strips;
    uint16_t first_strip_px;
    uint16_t last_strip_px;
};

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
    
    std::ifstream _file;

public:
    Cr2Reader(const char* path);
    
    int get_width();
    int get_height();
    
    double get_min_val();
    double get_max_val();
    
    void Parse();
    RawSensel* Process();
};

#endif
