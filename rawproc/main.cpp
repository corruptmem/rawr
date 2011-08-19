#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>

#include "Cr2Reader.h"
#include "GammaLookupTree.h"
#include "cl.hpp"

void processMultiImage(std::string images[], int image_count) {
    RawSensel** sensel = new RawSensel*[image_count];
    
    int w = 0;
    int h = 0;
    double min_val = -1;
    double max_val = -1;
    
    for(int i = 0; i<image_count; i++) {
        Cr2Reader parser(images[i].c_str());
        sensel[i] = parser.Process();
        w = parser.get_width();
        h = parser.get_height();
    }
    
    float* px = new float[w*h];
    
    for(int i = 0; i<w*h; i++) {
        double total_prob = 0;
        for(int img = 0; img<image_count; img++) {
            total_prob += sensel[img][i].prob;
        }
        
        if(total_prob == 0) total_prob = 1;
        
        double val = 0;
        for(int img = 0; img<image_count; img++) {
            val += sensel[img][i].val * (sensel[img][i].prob / total_prob);
        }
        
        min_val = (val < min_val || min_val == -1) ? val : min_val;
        max_val = (val > max_val || max_val == -1) ? val : max_val;
        
        px[i] = val;
    }
    
    for(int i = 0; i<image_count; i++) {
        delete sensel[i];
    }
    
    delete sensel;
    
    // opencl start
    
    
    std::vector<cl::Platform> platforms;
    
    cl::Platform::get(&platforms);
    
    cl_context_properties cps[3] = {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)(platforms[0])(),
        0
    };
    
    
    cl::Context context( CL_DEVICE_TYPE_GPU, cps);

    std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    
    // Create a command queue and use the first device
    cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);
    
    
    std::ifstream blurSource("blur.cl");
    std::string sourceCode((std::istreambuf_iterator<char>(blurSource)), 
                           std::istreambuf_iterator<char>());
    
    cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
    
    // Make program of the source code in the context
    cl::Program program = cl::Program(context, source);
    
    // Build program for these specific devices
    program.build(devices);
    
    // Make kernel
    cl::Kernel kernel(program, "blur");
    
    cl::Buffer buff_in = cl::Buffer(context, CL_MEM_READ_ONLY, w*h*sizeof(float));
    cl::Buffer buff_out = cl::Buffer(context, CL_MEM_WRITE_ONLY, w*h*3*sizeof(float));
    
    queue.enqueueWriteBuffer(buff_in, CL_TRUE, 0, w*h*sizeof(float), px);
    delete px;
    kernel.setArg(0, buff_in);
    kernel.setArg(1, buff_out);
    
    cl::NDRange global(h-8,w-8); //17907265
    cl::NDRange local(1,1);
    cl::NDRange offset(4,4);
    
    std::cout<<queue.enqueueNDRangeKernel(kernel, offset, global, local)<<std::endl;
    
    std::cout<<"Finishing.."<<std::endl;
    
    queue.finish();
    
    std::cout<<"Read log"<<std::endl;
    px = new float[w*h*3];
    queue.enqueueReadBuffer(buff_out, CL_TRUE, 0, w*h*3*sizeof(float), px);
    queue.finish();
    
    // opencl end
    
    std::cout<<"w: "<<w<<std::endl;
    std::cout<<"h: "<<h<<std::endl;
    std::cout<<"max_val: "<<max_val<<std::endl;
    std::cout<<"min_val: "<<min_val<<std::endl;
    
    GammaLookupTree glt(1.0/0.45, 256);
  //  GammaLookupTree glt(1.0/0.45, 65536);
    
    std::ofstream out("/tmp/out.ppm");
    out<<"P6"<<std::endl;
    out<<w<<std::endl;
    out<<h<<std::endl;
    out<<"255"<<std::endl;
    //out<<"65535"<<std::endl;
    
    for(int i = 0; i<w*h*3; i++) {
        double val = px[i];
        
        double scaled_val = ((val-min_val)/(max_val-min_val));
        if(scaled_val < 0) scaled_val = 0;
        if(scaled_val > 1) scaled_val = 1;
        
        unsigned int scaled_val_i = glt.get(scaled_val);
        
      //  out.put((unsigned char)((scaled_val_i >> 8) & 0xFF));
        out.put((unsigned char)(scaled_val_i & 0xFF));
        
        
        
    }
    
    out.close();
}

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        //std::string c[] = {"./_MG_2084.CR2" , "./_MG_2086.CR2", "./_MG_2089.CR2"};
        std::string c[] = {"/Users/cameron/Pictures/IMG_0021.CR2" , "/Users/cameron/Pictures/IMG_0022.CR2", "/Users/cameron/Pictures/IMG_0023.CR2"};
        processMultiImage(c, 3);
    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

