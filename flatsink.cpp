#include "flatsink.h"
#include <stdlib.h>
#include <stdio.h>

int cols;
int rows;
bool compareIndex ( NeighborhoodIteratorType :: IndexType a, NeighborhoodIteratorType :: IndexType b ) { //not perfect but works to eliminate duplicate indices from vector
    return !( a[1]*cols + a[0] < b[1]*cols + b[0] );
}

double angl (double a,double b) {
    if ( ( a >=0 ) && ( b >0 ) )
        return std :: atan(a/b);
    else if  ( ( a > 0 ) && ( b < 0) )
        return M_PI - std :: abs( atan(a/b) ) ;
    else if  ( (a < 0 ) && ( b < 0 ) )
        return M_PI + std :: abs( atan (a/b) ) ;
    else if ( ( a < 0 ) && ( b > 0) )
        return 2*M_PI - std :: abs (atan( a/b  ) );
    else if ( (a > 0) && ( b == 0) )
        return M_PI;
    else if ( ( a == 0) && (b < 0) )
        return M_PI;
    else if ( (a < 0 ) && (b == 0))
        return 3.0/2.0*M_PI;
}




template <class T>
void writeImageFromBuffer(ReaderType :: Pointer reader, T* localBuffer, char* fileName, bool flag  ) {

    typedef otb :: Image<T, 2> ImageOutType;
    typedef otb :: ImageFileWriter<ImageOutType> WriterOutType;
    typedef otb::ImportImageFilter<ImageOutType> ImportFilterType;

    typename WriterOutType :: Pointer   writer = WriterOutType :: New();
    typename ImportFilterType::Pointer importFilter = ImportFilterType :: New();

    writer->SetFileName(fileName);


    int  numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;
    importFilter->SetRegion(  reader->GetOutput()->GetLargestPossibleRegion() );
    importFilter->SetOrigin( reader->GetOutput()->GetOrigin() );
    importFilter->SetSpacing( reader->GetOutput()->GetSpacing() );
    importFilter->SetImportPointer(localBuffer, numberOfPixels, flag );

    writer->SetInput( dynamic_cast<ImageOutType*>( importFilter->GetOutput() ) );
    writer->Update();
}

bool FlatSink :: checkFlatSink( mapIndexType *idMap ) {
    NeighborhoodIteratorType :: IndexType *idx;
    idx =  new  NeighborhoodIteratorType :: IndexType (neighIt->GetIndex() );
    idIt->SetLocation(*idx);
    if ( idIt->InBounds() ) {
        short *cnt;
        cnt = new short (0);

        int *i;
        i = new int;
        for ( *i = 0;  *i < 9;  (*i)++ )  {
            if ( *i != 4 ) {
                if ( neighIt->GetCenterPixel() <= neighIt->GetPixel( *i ) )
                    (*cnt)++;
            }
        }
        if (*cnt == 8) {//FS pixel
            idIt->SetCenterPixel( id );
            idMap->at( id ).push_back( *idx );

            for ( *i = 0;  *i < 9;  (*i) ++ )  {
                if (*i != 4) {
                    if ( ( idIt->GetPixel( *i ) == 0  ) && (neighIt->GetPixel (*i) > maskValue ) ) {
                        neighIt->SetLocation(  neighIt->GetIndex( *i )  );
                        checkFlatSink( idMap );
                        neighIt->SetLocation( *idx );
                        idIt->SetLocation( *idx );
                    }
                }
            }
            delete i;
            i = NULL;
            delete cnt;
            cnt = NULL;
            delete idx;
            idx = NULL;
            return true;

        }
        else {
            idIt->SetCenterPixel( -1 );
            delete i;
            i = NULL;
            delete cnt;
            cnt = NULL;
            delete idx;
            idx = NULL;
            return false;
        }
    }
}

FlatSink::~FlatSink() {
    delete neighIt;
    delete idIt;
    delete borderIt;
}

FlatSink::FlatSink() {

}

FlatSink::FlatSink(ReaderType :: Pointer pnt, double val): reader(pnt), maskValue(val) {

    reader->Update();
    id = 1;
    radius.Fill(1);
    size = reader->GetOutput()->GetLargestPossibleRegion().GetSize();


    numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;

    //reading image into tmpImage
    tmpImage = ImageType :: New();
    tmpImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    tmpImage->CopyInformation(reader->GetOutput());
    tmpImage->Allocate();

    ConstIteratorType inputIt(reader->GetOutput(), reader->GetOutput()->GetLargestPossibleRegion() );
    IteratorType      outputIt(tmpImage,         tmpImage->GetLargestPossibleRegion() );
    for (inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd(); ++inputIt, ++outputIt) {
        outputIt.Set(inputIt.Get());
    }
    ImageType::RegionType region ;

    //creating image to store the ids
    idImage = ImageType :: New();
    idImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    idImage->CopyInformation(reader->GetOutput());
    idImage->Allocate();

    //creating image to store the borders

    borderImage = ImageType :: New();
    borderImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    borderImage->CopyInformation(reader->GetOutput());
    borderImage->Allocate();

    //extracting the inner region of the image
    FaceCalculatorType faceCalculator;
    FaceCalculatorType::FaceListType faceList;

    faceList = faceCalculator(idImage, reader->GetOutput()->GetLargestPossibleRegion(), radius);


    innerAreaIt = faceList.begin();

    //setting the inner area of the id image to 0
    innerAreaIt = faceList.begin();
    IteratorType idIt;
    idIt = IteratorType(idImage, *innerAreaIt);

    this->borderIt = new NeighborhoodIteratorType( radius, borderImage, idImage->GetRequestedRegion() );

    /*
    for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt ) {
        idIt.Set( 0 );
    }
    */
    idImage->FillBuffer(0);

    /*
    for(borderIt->GoToBegin(); !borderIt->IsAtEnd(); borderIt->operator ++() )
        borderIt->SetCenterPixel(0);
    */
    borderImage->FillBuffer(0);
    //setting the frame of the id image to -1 thus to be ignored by the algorithm
    innerAreaIt++;
    for (innerAreaIt;  innerAreaIt !=faceList.end(); innerAreaIt.operator ++() ) {
        IteratorType idIt = IteratorType(idImage, *innerAreaIt);
        for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt)
            idIt.Set( -1 );
    }

    WriterType :: Pointer writer;
    writer = WriterType :: New();
    writer->SetFileName("check_me_now.tif");
    writer->SetInput(idImage);

    writer->Update();

    innerAreaIt = faceList.begin();
    rows = size[1];
    cols = size[0];
    neighIt = new  NeighborhoodIteratorType( radius, tmpImage, *innerAreaIt );
    this->idIt = new NeighborhoodIteratorType( radius, idImage, *innerAreaIt );
}


void FlatSink::fillSinks() {
    bool recompute = false;
    int reps = 0;
    bool exported = false;
    do {
        recompute = false;
        mapIndexType *borderMap, *idMap;
        idMap = new mapIndexType;
        borderMap = new mapIndexType;

        std :: vector<ConstIteratorType :: IndexType> k;
        (*idMap)[id] = k;

        for ( idIt->GoToBegin(), neighIt->GoToBegin(); !neighIt->IsAtEnd(); neighIt->operator ++(), idIt->operator ++() ) {

            if ( ( idIt->GetCenterPixel() == 0 ) && (neighIt->GetCenterPixel() > maskValue ) ) //candidate pixels. The checkFlatSink will use only those which are in bounds
                if ( checkFlatSink( idMap ) ) {// when the if is entered all flat/sink pixels beloning to the same id are determined
                    (*borderMap)[id] = k;
                    //setBorder(borderMap, idMap); //getting the border of each FAD
                    id++;
                    (*idMap)[id] = k;
                }

        }
        idMap->erase(id);

        /*
        if (idMap->size() < 10) {
        std :: stringstream ss;
        ss << reps;
        ss  <<".img";
        std :: cout << ss.str() <<"\n";
        WriterType :: Pointer   writer = WriterType :: New();
        writer->SetFileName( ss.str() );
        writer->SetInput(idImage );
        writer->Update();
       }
*/


        std :: cout <<"Number of detected flat-sink areas: " << idMap->size() <<"\n";

        if ( (idMap->size() == 7283) ) {
            std :: stringstream ss2;
            ss2 <<"id_check";
            ss2  <<".img";
            WriterType :: Pointer   writer2 = WriterType :: New();
            writer2->SetFileName( ss2.str() );
            writer2->SetInput(idImage );
            writer2->Update();
        }

        mapIndexType::iterator itMap, itBor;
        /*
        for( itMap = idMap->begin(), itBor=borderMap->begin(); itMap != idMap->end(); itMap++, itBor++) {
          std :: cout <<(*itMap).second.size() << "\t"<< (*itBor).second.size() << "\n";
        }
     */
        for( itMap = idMap->begin(), itBor=borderMap->begin(); itMap != idMap->end(); itMap++, itBor++) {//for each FAD

            //minimum and maximum border heights
            PixelType minHeight = 99999999, maxHeight = -99999999;
             int curID = (*itMap).first;
            for (register int i = 0; i < (*itMap).second.size(); i++ ) {
                neighIt->SetLocation( (*itMap).second[i] );
                idIt->SetLocation( (*itMap).second[i] );
                borderIt->SetLocation( (*itMap).second[i] );

                for (register int i = 0; i < 9; i++) {
                    if ( (  idIt->GetPixel( i ) != curID ) && (  borderIt->GetPixel( i ) != curID  )  )   {
                        borderMap->at( curID ).push_back( idIt->GetIndex(i) );
                        borderIt->SetPixel( i, curID );
                        PixelType tmp = neighIt->GetPixel(i);
                        if (minHeight > tmp)
                            minHeight = tmp;

                        if (maxHeight < tmp)
                            maxHeight = tmp;
                    }
                }
            }

            /*
            if ( (*itBor).second.size()>8 ) {
                std :: cout <<"id = " <<curID <<"\n";
                std :: stringstream ss2;
                ss2 <<curID <<"_BORDER";
                ss2  <<".img";
                WriterType :: Pointer   writer2 = WriterType :: New();
                writer2->SetFileName( ss2.str() );
                writer2->SetInput(borderImage );
                writer2->Update();

            }
            */

            for (register int i = 0; i < (*itMap).second.size(); i++) {
                neighIt->SetLocation((*itMap).second[i]);
                neighIt->SetCenterPixel(minHeight);
            }
            //if min and max heights of the border are the same - SPECIAL CASE 1
            if (minHeight == maxHeight) {
                //correcting the height of border pixels. This is done by using the height of the pixel in which each border pixel flows into
                for(register int i = 0; i < (*itBor).second.size(); i++ ) {
                    neighIt->SetLocation((*itBor).second[i]);
                    idIt->SetLocation( (*itBor).second[i] );
                    borderIt->SetLocation( (*itBor).second[i] );
                    //finding maximum slope - SFDidMap->at(id).begin()
                    PixelType flowHeight = 9999999;
                    float maxSlope = 0.0;
                    for (register int pos = 0; pos < 9; pos++) {
                        if   ( (  idIt->GetPixel(pos) < 1 ) && ( borderIt->GetPixel( pos ) == 0 ) )   {//the examined pixel is neither grouped nor border. This will suffice since the heights of the border pixels are the same
                            float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel( pos) ) / std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;
                            if ( maxSlope < tmpSlope ) {
                                maxSlope = tmpSlope;
                                flowHeight = neighIt->GetPixel(pos);
                            }
                        }
                    }
                    if (maxSlope > 0.0 ) {
                        neighIt->SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight );
                    }
                }

            }
            else {//Trying to perform linear interpolation
                //finding outlet/inflow pixels
                std :: vector <ConstIteratorType :: IndexType*> outlet, inflow;
                for (register int i = 0; i < (*itBor).second.size(); i++) {
                    neighIt->SetLocation((*itBor).second[i] );
                    if (neighIt->GetCenterPixel() == minHeight)
                        outlet.push_back( &(*itBor).second[i] );
                    else
                        inflow.push_back( &(*itBor).second[i] );
                }
                //for each fs pixel
                int unprocessedFSCounter = 0;
                std :: vector<ConstIteratorType :: IndexType>:: iterator kk;


                for ( std :: vector<ConstIteratorType :: IndexType>:: iterator fs = (*itMap).second.begin(); fs != (*itMap).second.end(); fs++ ) {
                    kk = fs;
                    PixelType finalHeights = std :: pow(10, 6); // final heights
                    //std :: cout << outlet.size() << "\t" << inflow.size() << std :: endl;
                    int outletToOutletCounter = 0;

                    for ( register int i = 0; i < outlet.size(); i++ ) {
                        double ang = std :: atan2 (  (*fs)[1] -  (*outlet[i])[1],  (*fs)[0] - (*outlet[i])[0] ) ;
                        double outD = std :: sqrt (std ::  pow ( (*fs)[1] - (*outlet[i])[1] , 2 ) +  std :: pow ( (*fs)[0] - (*outlet[i])[0] , 2) );
                        //determining if the OF line is inside the FAD
                        bool cont2fs = true;
                        //checking if the line from the outlet towards the pixel is completely inside the FAD

                        for ( register int m = 1;  (  (cont2fs == true) ); m++ ) {
                            ImageType::IndexType index;
                            index[0] = round ( (*outlet[i])[0] + m*0.1*cos(ang) );
                            index[1] = round ( (*outlet[i])[1] + m*0.1*sin(ang) );


                            idIt->SetLocation(index);
                            borderIt->SetLocation(index);

                            if  (  ( *outlet[i] == index )|| ( idIt->GetCenterPixel() == (*itBor).first  ) ) {//the line is passing through a pixel belonging to the group
                                if (index == *fs) { //if I reach the examined flat sink pixel
                                    cont2fs = false;
                                    //continue with extending the line from the fs towards the border
                                    bool cont2border = true;
                                    for (register int m = 1;  ( cont2border == true ); m++) {
                                        index[0] = round ( (*fs)[0] + m*0.1*cos(ang) );
                                        index[1] = round ( (*fs)[1] + m*0.1* sin(ang) );
                                        neighIt->SetLocation( index );
                                        idIt->SetLocation(index);
                                        borderIt->SetLocation(index);
                                        //checking if the pixel is inflow or outlet
                                        if ( ( borderIt->GetCenterPixel() == (*itBor).first ) && ( neighIt->GetCenterPixel() > minHeight ) ) { //condition for boundary and inflow pixel
                                            double inHeight = neighIt->GetCenterPixel();
                                            double inD = std :: sqrt (std ::  pow ( (*fs)[1] - index[1] , 2 ) +  std :: pow ( (*fs)[0] - index[0] , 2) );
                                            PixelType tmpH = (outD*inHeight + inD*minHeight) / (outD + inD);
                                            if (finalHeights > tmpH)
                                                finalHeights = tmpH;

                                            cont2border = false;
                                        }
                                        else if ( ( borderIt->GetCenterPixel() == (*itBor).first ) && ( neighIt->GetCenterPixel() == minHeight ) ) { //condition for boundary and outlet pixel - SPECIAL CASE 2
                                            outletToOutletCounter++;
                                            cont2border = false;
                                        }
                                    }
                                }
                            }
                            else { //in case when the line falls out of the FAD or the examined outlet
                                cont2fs = false;
                            }
                        }
                    }
                    if ( outletToOutletCounter == outlet.size() ) {
                        unprocessedFSCounter++;
                    }
                    else {

                        if  (finalHeights != std :: pow(10, 6) ) { //
                            neighIt->SetLocation(*fs);
                            neighIt->SetCenterPixel( finalHeights + ((double) rand() / (RAND_MAX))*0.01 ); // set the result of linear interpolation + ((double) rand() / (RAND_MAX))*0.5
                        }
                    }
                }
                if ( unprocessedFSCounter == (*itMap).second.size()   ) {//all fs pixels remained unprocessed since special case 2 applied
                    //recompute heights for all outlet pixels
                    for(register int i = 0; i < outlet.size(); i++ ) {
                        neighIt->SetLocation( *outlet[i] );

                        //finding maximum slope - SFD
                        PixelType flowHeight = 9999;
                        float maxSlope = 0.0;

                        for (register int pos = 0; pos < 9; pos++) {
                            if ( pos != 4) {
                                /*
                                idIt->SetLocation( neighIt->GetIndex( pos ) );
                                borderIt->SetLocation(  neighIt->GetIndex( pos )  );
                                */
                                if (idIt->IndexInBounds( pos )  ) {
                                    if  ( ( idIt->GetPixel(pos) < 1 ) && ( borderIt->GetPixel(pos) != (*itBor).first )  )  { //the examined pixel is neither grouped nor border. This suffice since we examine only outlet pixels, thus max slopes will occur only outside the border
                                        float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel(pos) )/std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;

                                        if ( maxSlope < tmpSlope   ) {
                                            maxSlope = tmpSlope;
                                            flowHeight = neighIt->GetPixel(pos);
                                        }
                                    }
                                }
                            }
                        }
                        //std :: cout << "\n" <<(*itBor).first << "\t"  <<(*itBor).second[i] <<"\t" <<maxSlope <<"\n";
                        if (maxSlope > 0.0 ) {
                            neighIt->SetLocation( *outlet[i] );
                            neighIt->SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight + ((double) rand() / (RAND_MAX))*0.01 );
                        }
                    }

                }
                /*
                for (register int pp = 0; pp < (*itMap).second.size(); pp++ ) {
                    idIt->SetLocation ( (*itMap).second[pp] );
                    for(register int j = 0; j < 9; j++)
                        idIt->SetPixel( j, 0 );
                }
                */
                outlet.clear();
                inflow.clear();
            }

            /*
            for (register int i = 0; i < (*itBor).second.size(); i++ ) {
                borderIt->SetLocation((*itBor).second[i]);
                borderIt->SetCenterPixel(0);
            }
            */



        }

        if (idMap->size() > 0) {
            recompute = true;

            idImage->FillBuffer(0);
            borderImage->FillBuffer(0);
        }

        idMap->clear();
        borderMap->clear();
        delete idMap;
        idMap = NULL;
        reps++;
        delete borderMap;
        borderMap = NULL;
        id = 1;
    } while(recompute);
}

void FlatSink :: setBorder ( mapIndexType *borderMap, mapIndexType *idMap )  {
    NeighborhoodIteratorType :: IndexType tmpIdx = idIt->GetIndex();

    for ( std :: vector<ConstIteratorType :: IndexType> :: iterator it = idMap->at(id).begin();    it != idMap->at(id).end(); it++  ) {
        idIt->SetLocation( *it );
        borderIt->SetLocation( *it );

        for (register int i = 0; i < 9; i++) {
            if ( (  idIt->GetPixel( i ) != id ) && (  borderIt->GetPixel( i ) != id  )  )   {
                borderMap->at(id).push_back( idIt->GetIndex(i) );
                borderIt->SetPixel( i, id );
            }
        }
    }
    idIt->SetLocation( tmpIdx );
}

void FlatSink::writeImage( std::string fileName ) {
    WriterType :: Pointer   writer = WriterType :: New();
    writer->SetFileName(fileName);
    writer->SetInput(tmpImage);
    writer->Update();

}

