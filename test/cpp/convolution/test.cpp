#include <FImage.h>

using namespace FImage;

int main(int argc, char **argv) {

    int W = 64*3, H = 64*3;

    Image<uint16_t> in(W, H);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            in(x, y) = rand() & 0xff;
        }
    }


    Var x("x"), y("y");

    Image<uint16_t> tent = {{1, 2, 1},
                            {2, 4, 2},
                            {1, 2, 1}};
    
    Func input("input");
    input(x, y) = in(clamp(x, 0, W), clamp(y, 0, H));

    Func blur("blur");
    RVar i, j; 
    blur(x, y) += tent(i, j) * input(x + i - 1, y + j - 1);

    if (use_gpu()) {
        Var tidx("threadidx");
        Var bidx("blockidx");
        Var tidy("threadidy");
        Var bidy("blockidy");
        
        blur.split(x, bidx, tidx, 16);
        blur.parallel(bidx);
        blur.parallel(tidx);
        blur.split(y, bidy, tidy, 16);
        blur.parallel(bidy);
        blur.parallel(tidy);
        blur.transpose(bidx,tidy);
    }

    // Take this opportunity to test tiling reductions
    Var xi, yi;
    blur.tile(x, y, xi, yi, 6, 6);
    blur.update().tile(x, y, xi, yi, 4, 4);

    Image<uint16_t> out = blur.realize(W, H);

    for (int y = 1; y < H-1; y++) {
        for (int x = 1; x < W-1; x++) {
            uint16_t correct = (1*in(x-1, y-1) + 2*in(x, y-1) + 1*in(x+1, y-1) + 
                                2*in(x-1, y)   + 4*in(x, y) +   2*in(x+1, y) +
                                1*in(x-1, y+1) + 2*in(x, y+1) + 1*in(x+1, y+1));
            if (out(x, y) != correct) {
                printf("out(%d, %d) = %d instead of %d\n", x, y, out(x, y), correct);
                return -1;
            }
        }
    }

    printf("Success!\n");

    return 0;

}
