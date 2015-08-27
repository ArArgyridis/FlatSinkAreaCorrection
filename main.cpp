#include <iostream>
#include <sys/resource.h>
#include "flatsink.h"



int main(int argc, char* argv[]) {

    if (argc < 4)
    {
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << " inputImageFile inputCurvatureFile  outputImageFile" << std::endl;
        return EXIT_FAILURE;
    }

    const rlim_t kStackSize = 160 * 1024 * 1024;   // min stack size = 160 MB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < kStackSize) {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }

    ReaderType :: Pointer reader = ReaderType :: New();
    ReaderType :: Pointer readerPlan = ReaderType :: New();
    //ImportFilterType::Pointer importFilter = ImportFilterType :: New();

    reader->SetFileName(argv[1]);
    readerPlan->SetFileName(argv[2]);


    FlatSink flt(reader);

    flt.fillSinks();
    flt.writeImage( std :: string( argv[3] ) );

    std :: cout << "ALL WELL\n";
}


