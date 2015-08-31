#include <iostream>
#include <sys/resource.h>
#include "flatsink.h"


int main (int argc, char* argv[]) {

    NeighborhoodIteratorType :: IndexType a,b,c,d,e,f,g,h,k;

      /*
      a[0]=39;
      a[1]=2;
      b[0] = 37;
      b[1] =1;
      c[0] = 39;
      c[1] = 2;
      d[0] = 37;
      d[1]=1;
      e[0]=39;
      e[1]=1;
      f[0]=37;
      f[1]=2;
      g[0]=39;
      g[1]=2;
      h[0]=37;
      h[1]=3;
      k[0]=38;
      k[1]=3;



      std::vector <NeighborhoodIteratorType :: IndexType> vec;
      vec.push_back(a);
      vec.push_back(b);
      vec.push_back(e);
      vec.push_back(c);
      vec.push_back(d);
      vec.push_back(f);
      vec.push_back(h);
      vec.push_back(k);

      std::sort (vec.begin(), vec.end(), compareIndex);

      for (register int i = 0; i < vec.size(); i++)
          std::cout <<vec[i] <<"\t";
      std::cout <<"\n";

    vec.erase( std:: unique( vec.begin(), vec.end() ), vec.end() );

      for (register int i = 0; i < vec.size(); i++)
          std::cout <<vec[i] <<"\t";
      std::cout <<"\n";


      std :: cout << compareIndex(a,b) <<"\t"<< true <<  "\n";
    */

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

    FlatSink flt(reader);
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


  FlatSink flt(reader);

  flt.fillSinks();
  flt.writeImage( std :: string( argv[3] ) );

  std :: cout << "ALL WELL\n";
}


