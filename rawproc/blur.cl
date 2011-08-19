__kernel void blur(__global float* input, __global float* output) {
    uint y = get_global_id(0);
    uint x = get_global_id(1);
    uint xs =  get_global_size(1)+get_global_offset(1)*2;
    uint opx = (y*xs+x)*3;
    
    if(y%2==0 && x%2==0) {
        output[opx+0] = input[(y+1)*xs+x];
        output[opx+1] = input[y*xs+x];
        output[opx+2] = input[y*xs+(x+1)];
    } else if(y%2==1 && x%2==1) {
        output[opx+0] = input[y*xs+x+1];
        output[opx+1] = input[y*xs+x];
        output[opx+2] = input[(y+1)*xs+x];
    }else if(y%2==0) {
        output[opx] = input[(y+1)*xs+(x+1)];
        output[opx+1] = input[y*xs+(x+1)];
        output[opx+2] = input[y*xs+x];   
    }else if(x%2==0) {
        output[opx] = input[y*xs+x];
        output[opx+1] = input[y*xs+(x+1)];
        output[opx+2] = input[(y+1)*xs+(x+1)];
    }
}