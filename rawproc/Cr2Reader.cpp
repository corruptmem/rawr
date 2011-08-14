#include <iostream>
#include <math.h>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include "Cr2Reader.h"

using namespace boost::accumulators;

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

bool Cr2Header::is_valid_header() {
    if(byte_order!= 0x4949) 
        return false;
    
    if(magic_word!= 0x002a)
        return false;
    
    if(tiff_offset != 0x10)
        return false;
    
    if(cr2_magic_word != 0x5243)
        return false;
    
    if(cr2_major_ver!=0x2)
        return false;
    
    if(cr2_minor_ver != 0x0)
        return false;
    
    return true;
}

struct scale_points {
    int scale_in_start;
    double scale_out_start;
    
    scale_points(
        int scale_in_start, 
        double scale_out_start) : 
            scale_in_start(scale_in_start),
            scale_out_start(scale_out_start) { }
};

Cr2Reader::Cr2Reader(const char* path) :
    _path(path),
    _file(path),
    _file_parsed(false) { 
    
        scale_points mappings[] = {
            scale_points(0, 0.0),
            scale_points(50, 0.1),
            scale_points(120, 0.3),
            scale_points(200, 0.5),
            scale_points(300, 0.6),
            scale_points(500, 0.7),
            scale_points(900, 0.9),
            scale_points(1100, 0.9),
            scale_points(2200, 0.8),
            scale_points(3000, 0.7),
            scale_points(3300, 0.6),
            scale_points(4200, 0.5),
            scale_points(5000, 0.4),
            scale_points(6500, 0.3),
            scale_points(8000, 0.2),
            scale_points(9000, 0.15),
            scale_points(12000, 0.1),
            scale_points(16384, 0.0)
        };
        
        int cur = 0;
        int count = sizeof(mappings)/sizeof(scale_points);
        scale_points* cur_map = mappings;
        
        for(int i = 0; i<65536; i++) {
            if(cur<(count-2) && i>(cur_map+1)->scale_in_start) {
                cur++;
                cur_map++;
            }
            
            int cur_in = cur_map->scale_in_start;
            double cur_out = cur_map->scale_out_start;
            
            int next_in;
            double next_out;
            
            if(cur<(count-2)) {
                next_in = (cur_map+1)->scale_in_start;
                next_out =(cur_map+1)->scale_out_start; 
            } else {
                next_in = 65536;
                next_out = 0.0;
            }
            
            double fact = (((double)i)-((double)cur_in))/(((double)next_in)-((double)cur_in)); 
            _base_probs[i] = (next_out - cur_out)*fact + cur_out;
        }
}

int Cr2Reader::get_width() {
    return _sensor_width;
}

int Cr2Reader::get_height() {
    return _sensor_height;
}

double Cr2Reader::get_min_val() {
    return _min_val;
}

double Cr2Reader::get_max_val() {
    return _max_val;
}

void Cr2Reader::Parse() {
    if(!_file.is_open()) {
        throw "Error: Could not open file!";
    }
    
    _file.seekg(0, std::ios_base::beg);
    
    Cr2Header header;
    _file.read((char*)&header, sizeof(Cr2Header));
    
    if(!header.is_valid_header()){
        throw "Error: Invalid header!";
    }
    
    // -- ifd#0 read 
    Cr2IfdHeader ifd0_header;
    _file.read((char*)&ifd0_header, sizeof(Cr2IfdHeader));
    
    uint32_t exif_ptr = 0;
    
    for(int i = 0; i<ifd0_header.entries_count; i++) {
        Cr2IfdEntry ifd_entry;
        _file.read((char*)&ifd_entry, sizeof(Cr2IfdEntry));
        switch(ifd_entry.tag_id) {
            case 34665:
                // exif
                exif_ptr = ifd_entry.val;
        }
    }
    
    uint32_t makernote_ptr = 0;
    uint32_t fnum_ptr = 0;
    uint32_t shutter_ptr = 0;
    
    if(exif_ptr != 0) {
        _file.seekg(exif_ptr, std::ios_base::beg);
        Cr2IfdHeader exif_ifd_header; 
        _file.read((char*) &exif_ifd_header, sizeof(Cr2IfdHeader));
        
        for(int i = 0; i<exif_ifd_header.entries_count; i++){
            Cr2IfdEntry exif_ifd;
            _file.read((char*) &exif_ifd, sizeof(Cr2IfdEntry));
            
            switch(exif_ifd.tag_id) {
                case 0x829d:
                    fnum_ptr = exif_ifd.val;
                    break;
                case 0x8827:
                    _sensitivity = exif_ifd.val;
                    break;
                case 0x829a:
                    shutter_ptr = exif_ifd.val;
                    break;
                case 0x927c:
                    makernote_ptr = exif_ifd.val;
                    break;
            }
        }
    }
    
    if(fnum_ptr != 0) {
        _file.seekg(fnum_ptr, std::ios_base::beg);
        
        uint32_t a;
        uint32_t b;
        
        _file.read((char*)&a, 4);
        _file.read((char*)&b, 4);
        
        _f_number = ((double)a)/((double)b);
    }
    
    if(shutter_ptr != 0) {
        _file.seekg(shutter_ptr, std::ios_base::beg);
        
        uint32_t a;
        uint32_t b;
        
        _file.read((char*)&a, 4);
        _file.read((char*)&b, 4);
        
        _exposure_time = ((double)a)/((double)b);
    }
    
    uint32_t sensor_info_ptr = 0;
    int sensor_info_count;
    
    if(makernote_ptr != 0) {
        _file.seekg(makernote_ptr, std::ios_base::beg);
        Cr2IfdHeader makernote_ifd_header;
        _file.read((char*) &makernote_ifd_header, sizeof(Cr2IfdHeader));
        
        for(int i = 0; i<makernote_ifd_header.entries_count; i++) {
            Cr2IfdEntry makernote_ifd;
            _file.read((char*) &makernote_ifd, sizeof(Cr2IfdEntry));
            
            switch(makernote_ifd.tag_id) {
                case 0x00e0:
                    sensor_info_ptr = makernote_ifd.val;
                    sensor_info_count = makernote_ifd.number_of_val;
                    break;
            }
        }
    }
    
    if(sensor_info_ptr != 0) {
        _file.seekg(sensor_info_ptr, std::ios_base::beg);
        uint16_t sensor_info[sensor_info_count];
        
        _file.read((char*) &sensor_info, sizeof(uint16_t)*sensor_info_count);
        
        _sensor_width = sensor_info[1];
        _sensor_height = sensor_info[2];
        _sensor_left_border = sensor_info[5];
        _sensor_top_border = sensor_info[6];
        _sensor_right_border = sensor_info[7];
        _sensor_bottom_border = sensor_info[8];
    }
    
    
    // -- / ifd#0
    
    
    _file.seekg(header.raw_ifd_offset, std::ios_base::beg);
    
    Cr2IfdHeader ifd_header;
    _file.read((char*)&ifd_header, sizeof(Cr2IfdHeader));
    
    uint32_t strip_offset;
    uint32_t strip_byte_counts;
    uint32_t slice_info_offset;
    
    for(int i = 0; i<ifd_header.entries_count; i++) {
        Cr2IfdEntry ifd_entry;
        _file.read((char*)&ifd_entry, sizeof(Cr2IfdEntry));
        
        switch(ifd_entry.tag_id) {
            case 0x103:
                if(ifd_entry.val!=6) throw "Err: Didn't understand compression type 6";
                break;
            case 0x111:
                strip_offset = ifd_entry.val;
                break;
            case 0x117:
                strip_byte_counts = ifd_entry.val;
                break;
            case 0xc640:
                slice_info_offset = ifd_entry.val;
        }
    }
    
    Cr2IfdFooter ifd_footer;
    _file.read((char*)&ifd_footer, sizeof(Cr2IfdFooter));
    
    Cr2Slices slices;
    _file.seekg(slice_info_offset, std::ios_base::beg);
    _file.read((char*)&slices, sizeof(Cr2Slices));
    
    _raw_ptr = strip_offset;
    _first_strip_px = slices.first_strip_px;
    _last_strip_px = slices.last_strip_px;
    _first_strip_count = slices.num_first_strips;
    
    _file_parsed = true;
}

RawSensel* Cr2Reader::Process() {
    Parse();
    
    _file.seekg(_raw_ptr, std::ios_base::beg);
    
    LJpeg jp(_file);
    //----------
    
    unsigned short *rp;
    
    int h = _sensor_height;
    int w = _sensor_width;
    
    int j_h = jp.get_height();
    int j_w = jp.get_width();
    int j_c = 4;
    
    RawSensel* pixels = new RawSensel[w*h];
    
    double exposure = _exposure_time;
    double aperture = _f_number;
    double sensitivity = _sensitivity;
    
    double ev = log2(pow(aperture,2))-log2(exposure)-log2(sensitivity/100);
    double kev = pow(2,ev);
    
    int next = 0;
    _min_val = 100000;
    _max_val = 0;
    
    for(int jrow = 0; jrow<j_h; jrow++) {
        rp = jp.row();
        
        for(int px_c = 0; px_c < j_w*j_c; px_c++) {
            int strip_num = next / (h * _first_strip_px);
            if(strip_num > 2) {
                strip_num = 2;
            }
            
            int x, y;
            if(strip_num != _first_strip_count) {
                y = (next / _first_strip_px) % h;
                x = next % _first_strip_px + _first_strip_px*strip_num;
            } else {
                int sub_count = _first_strip_count * _first_strip_px * h;
                int this_count = next - sub_count;
                y = this_count / _last_strip_px;
                x = this_count % _last_strip_px + _first_strip_px*strip_num;
            }
            
            int pxpos = y*w + x;
            
            if(pxpos > w*h || pxpos < 0) {
                break;
            }
            
            unsigned short pxv = rp[px_c];
            unsigned short  px = pxv-2048 < 1 ? 1 : pxv-2048;
            double val = kev*(((double)px)/1000);
            
            if(val < _min_val) _min_val = val;
            if(val > _max_val) _max_val = val;
            
            pixels[pxpos].val = val;
            pixels[pxpos].prob = _base_probs[px] / (sensitivity/100);
            
            next++;
        }
        
    }
    
    return pixels;
}

