#include <iostream>
#include <sys/resource.h>
#include "flatsink.h"


int main (int argc, char* argv[]) {

    const rlim_t kStackSize = 16 * 1024 * 1024;   // min stack size = 160 MB
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

    reader->SetFileName(argv[1]);

    FlatSink flt(reader, atof(argv[3]) );
    flt.fillSinks();
    flt.writeImage(argv[2]);

   // std::cout << angl(-2,3) << "\t" << atan2(-2,3) <<"\n";


  std :: cout <<"ALL WELL!!\n";
  return 0;

}


int __main(int argc, char* argv[]) {

  if (argc < 3)
    {
      std::cerr << "Usage: " << std::endl;
      std::cerr << argv[0] << " inputImageFile  outputImageFile" << std::endl;
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


  FlatSink flt(reader , atof(argv[3]) );

  flt.fillSinks();
  flt.writeImage( std :: string( argv[3] ) );

  std :: cout << "ALL WELL\n";
}


