/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "cvtest.h"

class CV_FilterBaseTest : public CvArrTest
{
public:
    CV_FilterBaseTest( const char* test_name, const char* test_funcs, bool _fp_kernel );
    int write_default_params(CvFileStorage* fs);

protected:
    int support_testing_modes();
    int prepare_test_case( int test_case_idx );
    void prepare_to_validation( int /*test_case_idx*/ );
    int read_params( CvFileStorage* fs );
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    void get_minmax_bounds( int i, int j, int type, CvScalar* low, CvScalar* high );
    CvSize aperture_size;
    CvPoint anchor;
    int max_aperture_size;
    bool fp_kernel;
    bool inplace;
};


CV_FilterBaseTest::CV_FilterBaseTest( const char* test_name, const char* test_funcs, bool _fp_kernel )
    : CvArrTest( test_name, test_funcs, "" ), fp_kernel(_fp_kernel)
{
    test_array[INPUT].push(NULL);
    test_array[INPUT].push(NULL);
    test_array[TEMP].push(NULL);
    test_array[OUTPUT].push(NULL);
    test_array[REF_OUTPUT].push(NULL);
    max_aperture_size = 13;
    inplace = false;
    aperture_size = cvSize(0,0);
    anchor = cvPoint(0,0);
    element_wise_relative_error = false;
}


int CV_FilterBaseTest::read_params( CvFileStorage* fs )
{
    int code = CvTest::read_params( fs );
    if( code < 0 )
        return code;

    if( ts->get_testing_mode() == CvTS::CORRECTNESS_CHECK_MODE )
    {
        max_aperture_size = cvReadInt( find_param( fs, "max_aperture_size" ), max_aperture_size );
        max_aperture_size = cvTsClipInt( max_aperture_size, 1, 100 );
    }

    return code;
}


int CV_FilterBaseTest::write_default_params( CvFileStorage* fs )
{
    int code = CvArrTest::write_default_params( fs );
    if( code < 0 )
        return code;
    
    if( ts->get_testing_mode() == CvTS::CORRECTNESS_CHECK_MODE )
    {
        write_param( fs, "max_aperture_size", max_aperture_size );
    }

    return code;
}


void CV_FilterBaseTest::get_minmax_bounds( int i, int j, int type, CvScalar* low, CvScalar* high )
{
    CvArrTest::get_minmax_bounds( i, j, type, low, high );
    if( i == INPUT )
    {
        if( j == 1 )
        {
            if( fp_kernel )
            {
                if( ts->get_testing_mode() == CvTS::TIMING_MODE )
                {
                    *low = cvScalarAll(-1);
                    *high = cvScalarAll(1);
                }
                else
                {
                    CvRNG* rng = ts->get_rng();
                    double val = exp( cvTsRandReal(rng)*10 - 4 );
                    *low = cvScalarAll(-val);
                    *high = cvScalarAll(val);
                }
            }
            else
            {
                *low = cvScalarAll(0);
                *high = cvScalarAll(2);
            }
        }
        else if( CV_MAT_DEPTH(type) == CV_32F )
        {
            *low = cvScalarAll(-10.);
            *high = cvScalarAll(10.);
        }
    }
}


void CV_FilterBaseTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    int depth = test_case_idx*CV_32F/test_case_count;
    int cn = cvTsRandInt(rng) % 3 + 1;
    CvArrTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    depth += depth == CV_8S;
    cn += cn == 2;

    types[INPUT][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] = types[TEMP][0] = CV_MAKETYPE(depth, cn);

    aperture_size.width = cvTsRandInt(rng) % max_aperture_size + 1;
    aperture_size.height = cvTsRandInt(rng) % max_aperture_size + 1;
    anchor.x = cvTsRandInt(rng) % aperture_size.width;
    anchor.y = cvTsRandInt(rng) % aperture_size.height;

    types[INPUT][1] = fp_kernel ? CV_32FC1 : CV_8UC1;
    sizes[INPUT][1] = aperture_size;
    sizes[TEMP][0].width += aperture_size.width - 1;
    sizes[TEMP][0].height += aperture_size.height - 1;

    inplace = cvTsRandInt(rng) % 2 != 0;
}


int CV_FilterBaseTest::prepare_test_case( int test_case_idx )
{
    int code = CvArrTest::prepare_test_case( test_case_idx );
    if( code > 0 )
    {
        if( inplace && CV_ARE_TYPES_EQ(&test_mat[INPUT][0],&test_mat[OUTPUT][0]))
            cvTsCopy( &test_mat[INPUT][0], &test_mat[OUTPUT][0] );
        else
            inplace = false;
    }
    return code;
}


void CV_FilterBaseTest::prepare_to_validation( int /*test_case_idx*/ )
{
    CvMat* src = &test_mat[INPUT][0];
    
    if( !CV_ARE_TYPES_EQ( src, &test_mat[TEMP][0] ))
    {
        cvTsConvert( src, &test_mat[REF_OUTPUT][0] );
        src = &test_mat[REF_OUTPUT][0];
    }
    cvTsPrepareToFilter( src, &test_mat[TEMP][0],
                         anchor, CV_TS_BORDER_REPLICATE );
}


int CV_FilterBaseTest::support_testing_modes()
{
    return CvTS::CORRECTNESS_CHECK_MODE; // for now disable the timing test
}


CV_FilterBaseTest filter_base( "filter", "", false );

/////////////////////////

class CV_MorphologyBaseTest : public CV_FilterBaseTest
{
public:
    CV_MorphologyBaseTest( const char* test_name, const char* test_funcs, int optype );
    int write_default_params(CvFileStorage* fs);

protected:
    void prepare_to_validation( int test_case_idx );
    int prepare_test_case( int test_case_idx );
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    double get_success_error_level( int test_case_idx, int i, int j );
    int optype;
    int shape;
    IplConvKernel* element;
};


CV_MorphologyBaseTest::CV_MorphologyBaseTest( const char* test_name, const char* test_funcs, int _optype )
    : CV_FilterBaseTest( test_name, test_funcs, false ), optype(_optype)
{
    shape = -1;
    element = 0;
}


int CV_MorphologyBaseTest::write_default_params( CvFileStorage* fs )
{
    return CV_FilterBaseTest::write_default_params( fs );
}


void CV_MorphologyBaseTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CV_FilterBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = test_case_idx*2/test_case_count > 0 ? CV_32F : CV_8U;
    int cn = CV_MAT_CN(types[INPUT][0]);

    types[INPUT][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] = types[TEMP][0] = CV_MAKETYPE(depth, cn);
    shape = cvTsRandInt(rng) % 4;
    if( shape >= 3 )
        shape = CV_SHAPE_CUSTOM;
    else
        sizes[INPUT][1] = cvSize(0,0);
}


double CV_MorphologyBaseTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    return 0;
}


int CV_MorphologyBaseTest::prepare_test_case( int test_case_idx )
{
    int code = CV_FilterBaseTest::prepare_test_case( test_case_idx );
    int* eldata = 0;

    if( code <= 0 )
        return code;
    
    if( shape == CV_SHAPE_CUSTOM )
    {
        eldata = (int*)alloca( aperture_size.width*aperture_size.height*sizeof(eldata[0]) );
        uchar* src = test_mat[INPUT][1].data.ptr;
        int srcstep = test_mat[INPUT][1].step;
        int i, j, nonzero = 0;

        for( i = 0; i < aperture_size.height; i++ )
        {
            for( j = 0; j < aperture_size.width; j++ )
            {
                eldata[i*aperture_size.width + j] = src[i*srcstep + j];
                nonzero += src[i*srcstep + j] != 0;
            }
        }

        if( nonzero == 0 )
            eldata[anchor.y*aperture_size.width + anchor.x] = 1;
    }
    
    element = cvCreateStructuringElementEx( aperture_size.width, aperture_size.height,
                                            anchor.x, anchor.y, shape, eldata );
    return code;
}


void CV_MorphologyBaseTest::prepare_to_validation( int test_case_idx )
{
    CV_FilterBaseTest::prepare_to_validation( test_case_idx );
    cvTsMinMaxFilter( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], element, optype );
    cvReleaseStructuringElement( &element );
}


CV_MorphologyBaseTest morph( "morph", "", -1 );


/////////////// erode ///////////////

class CV_ErodeTest : public CV_MorphologyBaseTest
{
public:
    CV_ErodeTest();
protected:
    void run_func();
};


CV_ErodeTest::CV_ErodeTest()
    : CV_MorphologyBaseTest( "morph-erode", "cvErode", CV_TS_MIN )
{
}


void CV_ErodeTest::run_func()
{
    cvErode( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
             test_array[OUTPUT][0], element, 1 );
}

CV_ErodeTest erode_test;


/////////////// dilate ///////////////

class CV_DilateTest : public CV_MorphologyBaseTest
{
public:
    CV_DilateTest();
protected:
    void run_func();
};


CV_DilateTest::CV_DilateTest()
    : CV_MorphologyBaseTest( "morph-dilate", "cvDilate", CV_TS_MAX )
{
}


void CV_DilateTest::run_func()
{
    cvDilate( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
             test_array[OUTPUT][0], element, 1 );
}

CV_DilateTest dilate_test;


/////////////// generic filter ///////////////

class CV_FilterTest : public CV_FilterBaseTest
{
public:
    CV_FilterTest();

protected:
    void prepare_to_validation( int test_case_idx );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    double get_success_error_level( int test_case_idx, int i, int j );
};


CV_FilterTest::CV_FilterTest()
    : CV_FilterBaseTest( "filter-generic", "cvFilter2D", true )
{
}


void CV_FilterTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CV_FilterBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = test_case_idx*3/test_case_count;
    int cn = CV_MAT_CN(types[INPUT][0]);
    depth = depth == 0 ? CV_8U : depth == 1 ? CV_16U : CV_32F;
    types[INPUT][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] = types[TEMP][0] = CV_MAKETYPE(depth, cn);
}


double CV_FilterTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 2 : depth <= CV_32S ? 32 :
           depth == CV_32F ? 1e-5 : 1e-10;
}


void CV_FilterTest::run_func()
{
    cvFilter2D( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
                test_array[OUTPUT][0], &test_mat[INPUT][1], anchor );
}


void CV_FilterTest::prepare_to_validation( int test_case_idx )
{
    CV_FilterBaseTest::prepare_to_validation( test_case_idx );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], &test_mat[INPUT][1], anchor );
}

CV_FilterTest filter;


////////////////////////

class CV_DerivBaseTest : public CV_FilterBaseTest
{
public:
    CV_DerivBaseTest( const char* test_name, const char* test_funcs );
protected:
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    double get_success_error_level( int test_case_idx, int i, int j );
    int _aperture_size;
};


CV_DerivBaseTest::CV_DerivBaseTest( const char* test_name, const char* test_funcs )
    : CV_FilterBaseTest( test_name, test_funcs, true )
{
    max_aperture_size = 7;
}


void CV_DerivBaseTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CV_FilterBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = test_case_idx*2/test_case_count;
    depth = depth == 0 ? CV_8U : CV_32F;
    types[INPUT][0] = CV_MAKETYPE(depth,1);
    types[TEMP][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] = CV_MAKETYPE(depth==CV_8U?CV_16S:CV_32F,1);
    _aperture_size = (cvTsRandInt(rng)%5)*2 - 1;
    sizes[INPUT][1] = aperture_size = cvSize(_aperture_size, _aperture_size);
}


double CV_DerivBaseTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 1 : 1e-5;
}


/////////////// sobel ///////////////

class CV_SobelTest : public CV_DerivBaseTest
{
public:
    CV_SobelTest();

protected:
    int prepare_test_case( int test_case_idx );
    void prepare_to_validation( int test_case_idx );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    int dx, dy;
    int origin;
    IplImage img;
    void* src;
};


CV_SobelTest::CV_SobelTest() : CV_DerivBaseTest( "filter-sobel", "cvSobel" )
{
    src = 0;
}


void CV_SobelTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CV_DerivBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int max_d = _aperture_size > 0 ? 2 : 1;
    origin = cvTsRandInt(rng) % 2;
    dx = cvTsRandInt(rng) % (max_d + 1);
    dy = cvTsRandInt(rng) % (max_d + 1 - dx);
    if( dx == 0 && dy == 0 )
        dx = 1;
    if( cvTsRandInt(rng) % 2 )
    {
        int t;
        CV_SWAP( dx, dy, t );
    }
    
    if( _aperture_size < 0 )
        aperture_size = cvSize(3, 3);
    else if( _aperture_size == 1 )
    {
        if( dx == 0 )
            aperture_size = cvSize(1, 3);
        else if( dy == 0 )
            aperture_size = cvSize(3, 1);
        else
        {
            _aperture_size = 3;
            aperture_size = cvSize(3, 3);
        }
    }
    else
        aperture_size = cvSize(_aperture_size, _aperture_size);
        
    sizes[INPUT][1] = aperture_size;
    sizes[TEMP][0].width = sizes[INPUT][0].width + aperture_size.width - 1;
    sizes[TEMP][0].height = sizes[INPUT][0].height + aperture_size.height - 1;
    anchor.x = aperture_size.width / 2;
    anchor.y = aperture_size.height / 2;
}


int CV_SobelTest::prepare_test_case( int test_case_idx )
{
    int code = CV_FilterBaseTest::prepare_test_case( test_case_idx );
    if( code > 0 )
    {
        if( origin )
        {
            src = inplace ? &test_mat[OUTPUT][0] : &test_mat[INPUT][0];
            cvGetImage( src, &img );
            img.origin = origin;
            src = &img;
        }
        else
            src = inplace ? test_array[OUTPUT][0] : test_array[INPUT][0];
    }
    return code;
}


void CV_SobelTest::run_func()
{
    cvSobel( src, test_array[OUTPUT][0], dx, dy, _aperture_size );
}


static void cvTsCalcSobelKernel1D( int order, int _aperture_size, int size, int* kernel )
{
    int i, j, oldval, newval;

    if( _aperture_size < 0 )
    {
        static const int scharr[] = { 3, 10, 3, -1, 0, 1 };
        assert( size == 3 );
        memcpy( kernel, scharr + order*3, 3*sizeof(scharr[0]));
        return;
    }

    memset( kernel + 1, 0, size * sizeof(kernel[0]));
    kernel[0] = 1;

    for( i = 0; i < size - order - 1; i++ )
    {
        oldval = kernel[0];
        for( j = 1; j <= size; j++ )
        {
            newval = kernel[j] + kernel[j-1];
            kernel[j-1] = oldval;
            oldval = newval;
        }
    }

    for( i = 0; i < order; i++ )
    {
        oldval = -kernel[0];
        for( j = 1; j <= size; j++ )
        {
            newval = kernel[j-1] - kernel[j];
            kernel[j-1] = oldval;
            oldval = newval;
        }
    }
}


static void cvTsCalcSobelKernel2D( int dx, int dy, int _aperture_size, int origin, CvMat* kernel )
{
    int i, j;
    int* kx = (int*)alloca( (kernel->cols+1)*sizeof(kx[0]) );
    int* ky = (int*)alloca( (kernel->rows+1)*sizeof(ky[0]) );

    cvTsCalcSobelKernel1D( dx, _aperture_size, kernel->cols, kx );
    cvTsCalcSobelKernel1D( dy, _aperture_size, kernel->rows, ky );

    for( i = 0; i < kernel->rows; i++ )
    {
        float* kdata = (float*)(kernel->data.ptr + i*kernel->step);
        float ay = (float)ky[i]*(origin && (dy & 1) ? -1 : 1);
        for( j = 0; j < kernel->cols; j++ )
        {
            kdata[j] = kx[j]*ay;
        }
    }
}


void CV_SobelTest::prepare_to_validation( int test_case_idx )
{
    CV_DerivBaseTest::prepare_to_validation( test_case_idx );
    cvTsCalcSobelKernel2D( dx, dy, _aperture_size, origin, &test_mat[INPUT][1] );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], &test_mat[INPUT][1], anchor );
}

CV_SobelTest sobel_test;


/////////////// laplace ///////////////

class CV_LaplaceTest : public CV_DerivBaseTest
{
public:
    CV_LaplaceTest();

protected:
    void prepare_to_validation( int test_case_idx );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
};


CV_LaplaceTest::CV_LaplaceTest() : CV_DerivBaseTest( "filter-laplace", "cvLaplace" )
{
}


void CV_LaplaceTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CV_DerivBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    if( _aperture_size <= 1 )
    {
        if( _aperture_size < 0 )
            _aperture_size = 1;
        aperture_size = cvSize(3, 3);
    }
    else
        aperture_size = cvSize(_aperture_size, _aperture_size);
        
    sizes[INPUT][1] = aperture_size;
    sizes[TEMP][0].width = sizes[INPUT][0].width + aperture_size.width - 1;
    sizes[TEMP][0].height = sizes[INPUT][0].height + aperture_size.height - 1;
    anchor.x = aperture_size.width / 2;
    anchor.y = aperture_size.height / 2;
}


void CV_LaplaceTest::run_func()
{
    cvLaplace( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
               test_array[OUTPUT][0], _aperture_size );
}


static void cvTsCalcLaplaceKernel2D( int _aperture_size, CvMat* kernel )
{
    int i, j;
    int* kx = (int*)alloca( (kernel->cols+1)*sizeof(kx[0]) );
    int* ky = (int*)alloca( (kernel->rows+1)*sizeof(ky[0]) );

    cvTsCalcSobelKernel1D( 2, _aperture_size, kernel->cols, kx );
    if( _aperture_size > 1 )
        cvTsCalcSobelKernel1D( 0, _aperture_size, kernel->rows, ky );
    else
        ky[0] = ky[2] = 0, ky[1] = 1;

    for( i = 0; i < kernel->rows; i++ )
    {
        float* kdata = (float*)(kernel->data.ptr + i*kernel->step);
        for( j = 0; j < kernel->cols; j++ )
        {
            kdata[j] = (float)(kx[j]*ky[i] + kx[i]*ky[j]);
        }
    }
}


void CV_LaplaceTest::prepare_to_validation( int test_case_idx )
{
    CV_DerivBaseTest::prepare_to_validation( test_case_idx );
    cvTsCalcLaplaceKernel2D( _aperture_size, &test_mat[INPUT][1] );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], &test_mat[INPUT][1], anchor );
}


CV_LaplaceTest laplace_test;


////////////////////////////////////////////////////////////

class CV_SmoothBaseTest : public CV_FilterBaseTest
{
public:
    CV_SmoothBaseTest( const char* test_name, const char* test_funcs );

protected:
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    double get_success_error_level( int test_case_idx, int i, int j );
};


CV_SmoothBaseTest::CV_SmoothBaseTest( const char* test_name, const char* test_funcs )
    : CV_FilterBaseTest( test_name, test_funcs, true )
{
}


void CV_SmoothBaseTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CV_FilterBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = test_case_idx*2/test_case_count;
    int cn = CV_MAT_CN(types[INPUT][0]);
    depth = depth == 0 ? CV_8U : CV_32F;
    types[INPUT][0] = types[TEMP][0] =
        types[OUTPUT][0] = types[REF_OUTPUT][0] = CV_MAKETYPE(depth,cn);
    anchor.x = cvTsRandInt(rng)%(max_aperture_size/2+1);
    anchor.y = cvTsRandInt(rng)%(max_aperture_size/2+1);
    aperture_size.width = anchor.x*2 + 1;
    aperture_size.height = anchor.y*2 + 1;
    sizes[INPUT][1] = aperture_size;
    sizes[TEMP][0].width = sizes[INPUT][0].width + aperture_size.width - 1;
    sizes[TEMP][0].height = sizes[INPUT][0].height + aperture_size.height - 1;
}


double CV_SmoothBaseTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 1 : 1e-5;
}


/////////////// blur ///////////////

class CV_BlurTest : public CV_SmoothBaseTest
{
public:
    CV_BlurTest();

protected:
    void prepare_to_validation( int test_case_idx );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    bool normalize;
};


CV_BlurTest::CV_BlurTest() : CV_SmoothBaseTest( "filter-blur", "cvSmooth" )
{
}


void CV_BlurTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CV_SmoothBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    normalize = cvTsRandInt(rng) % 2 != 0;
    if( !normalize )
    {
        int depth = CV_MAT_DEPTH(types[INPUT][0]);
        types[INPUT][0] = CV_MAKETYPE(depth, 1); 
        types[TEMP][0] = types[OUTPUT][0] =
            types[REF_OUTPUT][0] = CV_MAKETYPE(depth==CV_8U?CV_16S:CV_32F,1);
    }
}


void CV_BlurTest::run_func()
{
    cvSmooth( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
              test_array[OUTPUT][0], normalize ? CV_BLUR : CV_BLUR_NO_SCALE,
              aperture_size.width, aperture_size.height );
}


void CV_BlurTest::prepare_to_validation( int test_case_idx )
{
    CvMat* kernel = &test_mat[INPUT][1];
    CV_SmoothBaseTest::prepare_to_validation( test_case_idx );
    cvTsAdd( 0, cvScalarAll(0.), 0, cvScalarAll(0.),
        cvScalarAll(normalize ? 1./(kernel->rows*kernel->cols) : 1.), kernel, 0 );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], kernel, anchor );
}


CV_BlurTest blur_test;


/////////////// gaussian ///////////////

class CV_GaussianBlurTest : public CV_SmoothBaseTest
{
public:
    CV_GaussianBlurTest();

protected:
    void prepare_to_validation( int test_case_idx );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    double get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ );
    double sigma;
    int param1, param2;
};


CV_GaussianBlurTest::CV_GaussianBlurTest() : CV_SmoothBaseTest( "filter-gaussian", "cvSmooth" )
{
    sigma = 0.;
}


double CV_GaussianBlurTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 2 : 1e-5;
}


void CV_GaussianBlurTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    int kernel_case = cvTsRandInt(rng) % 3;
    CV_SmoothBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    /*int cn = CV_MAT_CN(types[INPUT][0]);
    types[INPUT][0] = types[OUTPUT][0] = types[TEMP][0] =
        types[REF_OUTPUT][0] = CV_MAKETYPE(CV_32F,cn);*/

    sigma = exp(cvTsRandReal(rng)*5-2);
    param1 = aperture_size.width;
    param2 = aperture_size.height;

    if( kernel_case == 0 )
        sigma = 0.;
    else if( kernel_case == 2 )
    {
        int depth = CV_MAT_DEPTH(types[INPUT][0]);
        // !!! Copied from cvSmooth, if this formula is changed in cvSmooth,
        // make sure to update this one too.
        aperture_size.width = cvRound(sigma*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
        aperture_size.height = aperture_size.width;
        anchor.x = aperture_size.width / 2;
        anchor.y = aperture_size.height / 2;
        sizes[INPUT][1] = aperture_size;
        sizes[TEMP][0].width = sizes[INPUT][0].width + aperture_size.width - 1;
        sizes[TEMP][0].height = sizes[INPUT][0].height + aperture_size.height - 1;
        param1 = param2 = 0;
    }
    inplace = true;
}


void CV_GaussianBlurTest::run_func()
{
    cvSmooth( inplace ? test_array[OUTPUT][0] : test_array[INPUT][0],
              test_array[OUTPUT][0], CV_GAUSSIAN, param1, param2, sigma );
}


// !!! Copied from cvSmooth, if the code is changed in cvSmooth,
// make sure to update this one too.
#define SMALL_GAUSSIAN_SIZE 7
static void
icvCalcGaussianKernel( int n, double sigma, float* kernel )
{
    static const float small_gaussian_tab[][SMALL_GAUSSIAN_SIZE] = 
    {
        {1.f},
        {0.25f, 0.5f, 0.25f},
        {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f},
        {0.03125, 0.109375, 0.21875, 0.28125, 0.21875, 0.109375, 0.03125}
    };

    if( n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 )
    {
        assert( n%2 == 1 );
        memcpy( kernel, small_gaussian_tab[n>>1], n*sizeof(kernel[0]));
    }
    else
    {
        double sigmaX = sigma > 0 ? sigma : (n/2 - 1)*0.3 + 0.8;
        double scale2X = -0.5/(sigmaX*sigmaX);
        double sum = 1.;
        int i;
        sum = kernel[n/2] = 1.f;
    
        for( i = 1; i <= n/2; i++ )
        {
            kernel[n/2+i] = kernel[n/2-i] = (float)exp(scale2X*i*i);
            sum += kernel[n/2+i]*2;
        }

        sum = 1./sum;
        for( i = 0; i <= n/2; i++ )
            kernel[n/2+i] = kernel[n/2-i] = (float)(kernel[n/2+i]*sum);
    }
}


static void cvTsCalcGaussianKernel2D( double sigma, CvMat* kernel )
{
    int i, j;
    float* kx = (float*)alloca( kernel->cols*sizeof(kx[0]) );
    float* ky = (float*)alloca( kernel->rows*sizeof(ky[0]) );

    icvCalcGaussianKernel( kernel->cols, sigma, kx );
    icvCalcGaussianKernel( kernel->rows, sigma, ky );

    for( i = 0; i < kernel->rows; i++ )
    {
        float* kdata = (float*)(kernel->data.ptr + i*kernel->step);
        for( j = 0; j < kernel->cols; j++ )
            kdata[j] = kx[j]*ky[i];
    }
}


void CV_GaussianBlurTest::prepare_to_validation( int test_case_idx )
{
    CvMat* kernel = &test_mat[INPUT][1];
    CV_SmoothBaseTest::prepare_to_validation( test_case_idx );
    cvTsCalcGaussianKernel2D( sigma, &test_mat[INPUT][1] );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0], kernel, anchor );
}


CV_GaussianBlurTest gaussianblur_test;


/////////////// median ///////////////

class CV_MedianBlurTest : public CV_SmoothBaseTest
{
public:
    CV_MedianBlurTest();

protected:
    void prepare_to_validation( int test_case_idx );
    double get_success_error_level( int test_case_idx, int i, int j );
    void run_func();
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
};


CV_MedianBlurTest::CV_MedianBlurTest() : CV_SmoothBaseTest( "filter-median", "cvSmooth" )
{
    test_array[TEMP].push(NULL);
}


void CV_MedianBlurTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CV_SmoothBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = CV_8U;
    int cn = CV_MAT_CN(types[INPUT][0]);
    types[INPUT][0] = types[TEMP][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] = CV_MAKETYPE(depth,cn);
    types[INPUT][1] = types[TEMP][0] = types[TEMP][1] = CV_MAKETYPE(depth,1);
    
    aperture_size.height = aperture_size.width;
    anchor.x = anchor.y = aperture_size.width / 2;
    sizes[INPUT][1] = cvSize(aperture_size.width,aperture_size.height);

    sizes[INPUT][0].width = MAX( sizes[INPUT][0].width, aperture_size.width );
    sizes[INPUT][0].height = MAX( sizes[INPUT][0].height, aperture_size.width );
    sizes[OUTPUT][0] = sizes[REF_OUTPUT][0] = sizes[INPUT][0];

    sizes[TEMP][0].width = sizes[INPUT][0].width + aperture_size.width - 1;
    sizes[TEMP][0].height = sizes[INPUT][0].height + aperture_size.height - 1;
    
    sizes[TEMP][1] = cn > 1 ? sizes[INPUT][0] : cvSize(0,0);
    inplace = false;
}


double CV_MedianBlurTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    return 0;
}


void CV_MedianBlurTest::run_func()
{
    cvSmooth( test_array[INPUT][0], test_array[OUTPUT][0],
              CV_MEDIAN, aperture_size.width );
}


struct median_pair
{
    int col;
    int val;
    median_pair() {};
    median_pair( int _col, int _val ) : col(_col), val(_val) {};
};


static void cvTsMedianFilter( const CvMat* src, CvMat* dst, int m )
{
    int i, j, k, l, m2 = m*m, n;
    int* col_buf = (int*)malloc( (m+1)*sizeof(col_buf[0]));
    median_pair* buf0 = (median_pair*)malloc( (m*m+1)*sizeof(buf0[0]));
    median_pair* buf1 = (median_pair*)malloc( (m*m+1)*sizeof(buf1[0]));
    median_pair* tbuf;
    int step = src->step/CV_ELEM_SIZE(src->type);

    assert( src->rows == dst->rows + m - 1 && src->cols == dst->cols + m - 1 &&
            CV_ARE_TYPES_EQ(src,dst) && CV_MAT_TYPE(src->type) == CV_8UC1 );

    for( i = 0; i < dst->rows; i++ )
    {
        uchar* dst1 = (uchar*)(dst->data.ptr + dst->step*i);
        for( k = 0; k < m; k++ )
        {
            const uchar* src1 = (const uchar*)(src->data.ptr + (i+k)*src->step);
            for( j = 0; j < m-1; j++ )
                *buf0++ = median_pair(j, src1[j]);
        }

        n = m2 - m;
        buf0 -= n;
        for( k = n-1; k > 0; k-- )
        {
            int f = 0;
            for( j = 0; j < k; j++ )
            {
                if( buf0[j].val > buf0[j+1].val )
                {
                    median_pair t;
                    CV_SWAP( buf0[j], buf0[j+1], t );
                    f = 1;
                }
            }
            if( !f )
                break;
        }

        for( j = 0; j < dst->cols; j++ )
        {
            int ins_col = j + m - 1;
            int del_col = j - 1;
            const uchar* src1 = (const uchar*)(src->data.ptr + src->step*i) + ins_col;
            for( k = 0; k < m; k++, src1 += step )
            {
                col_buf[k] = src1[0];
                for( l = k-1; l >= 0; l-- )
                {
                    int t;
                    if( col_buf[l] < col_buf[l+1] )
                        break;
                    CV_SWAP( col_buf[l], col_buf[l+1], t );
                }
            }

            col_buf[m] = INT_MAX;

            for( k = 0, l = 0; k < n; )
            {
                if( buf0[k].col == del_col )
                    k++;
                else if( buf0[k].val < col_buf[l] )
                    *buf1++ = buf0[k++];
                else
                {
                    assert( col_buf[l] < INT_MAX );
                    *buf1++ = median_pair(ins_col,col_buf[l++]);
                }
            }

            for( ; l < m; l++ )
                *buf1++ = median_pair(ins_col,col_buf[l]);

            if( del_col < 0 )
                n += m;
            buf1 -= n;
            assert( n == m2 );
            dst1[j] = (uchar)buf1[n/2].val;
            CV_SWAP( buf0, buf1, tbuf );
        }
    }

    free(col_buf);
    free(buf0);
    free(buf1);
}


void CV_MedianBlurTest::prepare_to_validation( int /*test_case_idx*/ )
{
    // CV_SmoothBaseTest::prepare_to_validation( test_case_idx );
    CvMat* src0 = &test_mat[INPUT][0];
    CvMat* dst0 = &test_mat[REF_OUTPUT][0];
    int i, cn = CV_MAT_CN(src0->type);
    CvMat* src = &test_mat[TEMP][0], *dst = dst0;
    if( cn > 1 )
        dst = &test_mat[TEMP][1];

    for( i = 0; i < cn; i++ )
    {
        CvMat* ptr = src0;
        if( cn > 1 )
        {
            cvTsExtract( src0, dst, i );
            ptr = dst;
        }
        cvTsPrepareToFilter( ptr, src, anchor, CV_TS_BORDER_REPLICATE );
        cvTsMedianFilter( src, dst, aperture_size.width );
        if( cn > 1 )
            cvTsInsert( dst, dst0, i );
    }
}


CV_MedianBlurTest medianblur_test;


/////////////// pyramid tests ///////////////

class CV_PyramidBaseTest : public CV_FilterBaseTest
{
public:
    CV_PyramidBaseTest( const char* test_name, const char* test_funcs, bool downsample );

protected:
    int prepare_test_case( int test_case_idx );
    double get_success_error_level( int test_case_idx, int i, int j );
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    bool downsample;
};


CV_PyramidBaseTest::CV_PyramidBaseTest( const char* test_name, const char* test_funcs, bool _downsample )
    : CV_FilterBaseTest( test_name, test_funcs, true ), downsample(_downsample)
{
    test_array[TEMP].push(NULL);
}


double CV_PyramidBaseTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 1 : 1e-5;
}


void CV_PyramidBaseTest::get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CvSize sz;
    CV_FilterBaseTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );

    int depth = test_case_idx*2/test_case_count > 0 ? CV_32F : CV_8U;
    int cn = cvTsRandInt(rng) & 1 ? 3 : 1;

    aperture_size = cvSize(5,5);
    anchor = cvPoint(aperture_size.width/2, aperture_size.height/2);

    types[INPUT][0] = types[OUTPUT][0] = types[REF_OUTPUT][0] =
        types[TEMP][0] = types[TEMP][1] = CV_MAKETYPE(depth, cn);

    sz.width = MAX( sizes[INPUT][0].width/2, 1 );
    sz.height = MAX( sizes[INPUT][0].height/2, 1 );

    if( downsample )
    {
        sizes[OUTPUT][0] = sizes[REF_OUTPUT][0] = sz;
        sz.width *= 2;
        sz.height *= 2;
        sizes[INPUT][0] = sizes[TEMP][1] = sz;
    }
    else
    {
        sizes[INPUT][0] = sz;
        sz.width *= 2;
        sz.height *= 2;
        sizes[OUTPUT][0] = sizes[REF_OUTPUT][0] = sz;
        sizes[TEMP][1] = cvSize(0,0);
    }
    
    sizes[INPUT][1] = aperture_size;
    sizes[TEMP][0].width = sz.width + aperture_size.width - 1;
    sizes[TEMP][0].height = sz.height + aperture_size.height - 1;
    inplace = false;
}


int CV_PyramidBaseTest::prepare_test_case( int test_case_idx )
{
    static const float kdata[] = { 1.f, 4.f, 6.f, 4.f, 1.f };
    int i, j;
    double scale = 1./256;
    CvMat* kernel;
    int code = CV_FilterBaseTest::prepare_test_case( test_case_idx );
    
    if( code <= 0 )
        return code;

    if( !downsample )
        scale *= 4;

    kernel = &test_mat[INPUT][1];

    for( i = 0; i < aperture_size.height; i++ )
    {
        float* krow = (float*)(kernel->data.ptr + i*kernel->step);
        for( j = 0; j < aperture_size.width; j++ )
            krow[j] = (float)(scale*kdata[i]*kdata[j]);
    }
    return code;
}


CV_PyramidBaseTest pyr_base( "pyramid", "", false );


/////// pyrdown ////////

static void cvTsDownsample( const CvMat* src, CvMat* dst )
{
    int i, j, k;
    int elem_size = CV_ELEM_SIZE(src->type);
    int ncols = dst->cols*elem_size;
    int is_dword = elem_size % sizeof(int) == 0;

    if( is_dword )
    {
        elem_size /= sizeof(int);
        ncols /= sizeof(int);
    }
    
    for( i = 0; i < dst->rows; i++ )
    {
        const uchar* src_row = src->data.ptr + i*2*src->step;
        uchar* dst_row = dst->data.ptr + i*dst->step;

        if( !is_dword )
        {
            for( j = 0; j < ncols; j += elem_size )
            {
                for( k = 0; k < elem_size; k++ )
                    dst_row[j+k] = src_row[j*2+k];
            }
        }
        else
        {
            for( j = 0; j < ncols; j += elem_size )
            {
                for( k = 0; k < elem_size; k++ )
                    ((int*)dst_row)[j+k] = ((const int*)src_row)[j*2+k];
            }
        }
    }
}


class CV_PyramidDownTest : public CV_PyramidBaseTest
{
public:
    CV_PyramidDownTest();

protected:
    void run_func();
    void prepare_to_validation( int );
};


CV_PyramidDownTest::CV_PyramidDownTest()
    : CV_PyramidBaseTest( "pyramid-down", "cvPyrDown", true )
{
}


void CV_PyramidDownTest::run_func()
{
    cvPyrDown( test_array[INPUT][0], test_array[OUTPUT][0], CV_GAUSSIAN_5x5 );
}


void CV_PyramidDownTest::prepare_to_validation( int /*test_case_idx*/ )
{
    cvTsPrepareToFilter( &test_mat[INPUT][0], &test_mat[TEMP][0],
                         anchor, CV_TS_BORDER_REFLECT );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[TEMP][1],
                    &test_mat[INPUT][1], anchor );
    cvTsDownsample( &test_mat[TEMP][1], &test_mat[REF_OUTPUT][0] );
}


CV_PyramidDownTest pyrdown;


/////// pyrup ////////

static void cvTsUpsample( const CvMat* src, CvMat* dst )
{
    int i, j, k;
    int elem_size = CV_ELEM_SIZE(src->type);
    int ncols = src->cols*elem_size;
    int is_dword = elem_size % sizeof(int) == 0;

    if( is_dword )
    {
        elem_size /= sizeof(int);
        ncols /= sizeof(int);
    }
    
    for( i = 0; i < src->rows; i++ )
    {
        const uchar* src_row = src->data.ptr + i*src->step;
        uchar* dst_row = dst->data.ptr + i*2*dst->step;

        if( !is_dword )
        {
            memset( dst_row + dst->step, 0, dst->cols*elem_size );
            for( j = 0; j < ncols; j += elem_size )
            {
                for( k = 0; k < elem_size; k++ )
                {
                    dst_row[j*2+k] = src_row[j+k];
                    dst_row[j*2+k+elem_size] = 0;
                }
            }
        }
        else
        {
            memset( dst_row + dst->step, 0, dst->cols*elem_size*sizeof(int) );
            for( j = 0; j < ncols; j += elem_size )
            {
                for( k = 0; k < elem_size; k++ )
                {
                    ((int*)dst_row)[j*2+k] = ((const int*)src_row)[j+k];
                    ((int*)dst_row)[j*2+k+elem_size] = 0;
                }
            }
        }
    }
}


class CV_PyramidUpTest : public CV_PyramidBaseTest
{
public:
    CV_PyramidUpTest();

protected:
    void run_func();
    void prepare_to_validation( int );
};


CV_PyramidUpTest::CV_PyramidUpTest()
    : CV_PyramidBaseTest( "pyramid-up", "cvPyrUp", false )
{
}


void CV_PyramidUpTest::run_func()
{
    cvPyrUp( test_array[INPUT][0], test_array[OUTPUT][0], CV_GAUSSIAN_5x5 );
}


void CV_PyramidUpTest::prepare_to_validation( int /*test_case_idx*/ )
{
    CvMat src2, dst2;
    CvSize sz;
    cvTsUpsample( &test_mat[INPUT][0], &test_mat[REF_OUTPUT][0] );
    cvTsPrepareToFilter( &test_mat[REF_OUTPUT][0], &test_mat[TEMP][0],
                         anchor, CV_TS_BORDER_REFLECT );
    cvTsConvolve2D( &test_mat[TEMP][0], &test_mat[REF_OUTPUT][0],
                    &test_mat[INPUT][1], anchor );
    // currently IPP and OpenCV process right/bottom part of the image differently, so
    // we just patch the last two rows/columns to have consistent test results.
    sz = cvGetMatSize( &test_mat[REF_OUTPUT][0]);
    cvTsSelect( &test_mat[REF_OUTPUT][0], &src2, cvRect(sz.width-2,0,2,sz.height) );
    cvTsSelect( &test_mat[OUTPUT][0], &dst2, cvRect(sz.width-2,0,2,sz.height) );
    cvTsCopy( &src2, &dst2, 0 );
    cvTsSelect( &test_mat[REF_OUTPUT][0], &src2, cvRect(0,sz.height-2,sz.width,2) );
    cvTsSelect( &test_mat[OUTPUT][0], &dst2, cvRect(0,sz.height-2,sz.width,2) );
    cvTsCopy( &src2, &dst2, 0 );
}


CV_PyramidUpTest pyrup;



//////////////////////// feature selection //////////////////////////

class CV_FeatureSelBaseTest : public CvArrTest
{
public:
    CV_FeatureSelBaseTest( const char* test_name, const char* test_funcs, int width_factor );
    int write_default_params(CvFileStorage* fs);

protected:
    int support_testing_modes();
    int read_params( CvFileStorage* fs );
    void get_test_array_types_and_sizes( int test_case_idx, CvSize** sizes, int** types );
    void get_minmax_bounds( int i, int j, int type, CvScalar* low, CvScalar* high );
    double get_success_error_level( int test_case_idx, int i, int j );
    int aperture_size, block_size;
    int max_aperture_size;
    int max_block_size;
    int width_factor;
};


CV_FeatureSelBaseTest::CV_FeatureSelBaseTest( const char* test_name, const char* test_funcs, int _width_factor )
    : CvArrTest( test_name, test_funcs, "" )
{
    max_aperture_size = 7;
    max_block_size = 21;
    // 1 input, 1 output, temp arrays are allocated in the reference functions
    test_array[INPUT].push(NULL);
    test_array[OUTPUT].push(NULL);
    test_array[REF_OUTPUT].push(NULL);
    element_wise_relative_error = false;
    width_factor = _width_factor;
}


int CV_FeatureSelBaseTest::read_params( CvFileStorage* fs )
{
    int code = CvTest::read_params( fs );
    if( code < 0 )
        return code;

    if( ts->get_testing_mode() == CvTS::CORRECTNESS_CHECK_MODE )
    {
        max_aperture_size = cvReadInt( find_param( fs, "max_aperture_size" ), max_aperture_size );
        max_aperture_size = cvTsClipInt( max_aperture_size, 1, 9 );
        max_block_size = cvReadInt( find_param( fs, "max_block_size" ), max_block_size );
        max_block_size = cvTsClipInt( max_aperture_size, 1, 100 );
    }

    return code;
}


int CV_FeatureSelBaseTest::write_default_params( CvFileStorage* fs )
{
    int code = CvArrTest::write_default_params( fs );
    if( code < 0 )
        return code;
    
    if( ts->get_testing_mode() == CvTS::CORRECTNESS_CHECK_MODE )
    {
        write_param( fs, "max_aperture_size", max_aperture_size );
        write_param( fs, "max_block_size", max_block_size );
    }

    return code;
}


double CV_FeatureSelBaseTest::get_success_error_level( int /*test_case_idx*/, int /*i*/, int /*j*/ )
{
    int depth = CV_MAT_DEPTH(test_mat[INPUT][0].type);
    return depth <= CV_8S ? 1e-2 : depth == CV_32F ? 1e-3 : 1e-10;
}


void CV_FeatureSelBaseTest::get_minmax_bounds( int i, int j, int type, CvScalar* low, CvScalar* high )
{
    CvArrTest::get_minmax_bounds( i, j, type, low, high );
    if( i == INPUT && CV_MAT_DEPTH(type) == CV_32F )
    {
        *low = cvScalarAll(-10.);
        *high = cvScalarAll(10.);
    }
}


void CV_FeatureSelBaseTest::get_test_array_types_and_sizes( int test_case_idx,
                                                CvSize** sizes, int** types )
{
    CvRNG* rng = ts->get_rng();
    CvArrTest::get_test_array_types_and_sizes( test_case_idx, sizes, types );
    int depth = test_case_idx*2/test_case_count, asz;

    depth = depth == 0 ? CV_8U : CV_32F;
    types[INPUT][0] = depth;
    types[OUTPUT][0] = types[REF_OUTPUT][0] = CV_32FC1;

    aperture_size = (cvTsRandInt(rng) % (max_aperture_size+2) - 1) | 1;
    if( aperture_size == 1 )
        aperture_size = 3;
    if( depth == CV_8U )
        aperture_size = MIN( aperture_size, 5 );
    block_size = (cvTsRandInt(rng) % max_block_size + 1) | 1;
    if( block_size <= 3 )
        block_size = 3;
    asz = aperture_size > 0 ? aperture_size : 3;

    sizes[INPUT][0].width = MAX( sizes[INPUT][0].width, asz + block_size );
    sizes[INPUT][0].height = MAX( sizes[INPUT][0].height, asz + block_size );
    sizes[OUTPUT][0].height = sizes[REF_OUTPUT][0].height = sizes[INPUT][0].height;
    sizes[OUTPUT][0].width = sizes[REF_OUTPUT][0].width = sizes[INPUT][0].width*width_factor;
}


int CV_FeatureSelBaseTest::support_testing_modes()
{
    return CvTS::CORRECTNESS_CHECK_MODE; // for now disable the timing test
}


CV_FeatureSelBaseTest featuresel_base( "features", "", 0 );


static void
cvTsCornerEigenValsVecs( const CvMat* _src, CvMat* eigenv,
                         int block_size, int _aperture_size,
                         int mode )
{
    CvMat *dx2 = 0, *dxdy = 0, *dy2 = 0;
    CvMat* kernel = 0, *src2 = 0;
    const CvMat* src = _src;
    
    int type, ftype;
    double denom;

    CV_FUNCNAME( "cvTsCornerEigenValsVecs" );

    __BEGIN__;

    int i, j;
    int aperture_size = _aperture_size < 0 ? 3 : _aperture_size;
    CvPoint anchor = { aperture_size/2, aperture_size/2 };

    assert( (CV_MAT_TYPE(src->type) == CV_8UC1 ||
            CV_MAT_TYPE(src->type) == CV_32FC1) &&
            CV_MAT_TYPE(eigenv->type) == CV_32FC1 );

    assert( src->rows == eigenv->rows &&
            (mode > 0 && src->cols == eigenv->cols ||
            mode == 0 && src->cols*6 == eigenv->cols) );

    type = CV_MAT_TYPE(src->type);
    ftype = CV_32FC1;
    
    CV_CALL( dx2 = cvCreateMat( src->rows, src->cols, ftype ));
    CV_CALL( dy2 = cvCreateMat( src->rows, src->cols, ftype ));
    CV_CALL( dxdy = cvCreateMat( src->rows, src->cols, ftype ));

    CV_CALL( kernel = cvCreateMat( aperture_size, aperture_size, CV_32FC1 ));
    CV_CALL( src2 = cvCreateMat( src->rows + aperture_size - 1,
                                 src->cols + aperture_size - 1, ftype ));

    if( type != ftype )
    {
        cvTsAdd( src, cvScalarAll(1./255), 0, cvScalarAll(0.), cvScalarAll(0.), dx2, 0 );
        src = dx2;
    }
    
    cvTsPrepareToFilter( src, src2, anchor, CV_TS_BORDER_REPLICATE );
    cvTsCalcSobelKernel2D( 1, 0, _aperture_size, 0, kernel );
    cvTsConvolve2D( src2, dx2, kernel, anchor );
    cvTsCalcSobelKernel2D( 0, 1, _aperture_size, 0, kernel );
    cvTsConvolve2D( src2, dy2, kernel, anchor );
    cvReleaseMat( &src2 );
    cvReleaseMat( &kernel );

    denom = (1 << (aperture_size-1))*block_size;
    denom = denom * denom;
    if( _aperture_size < 0 )
        denom *= 4;
    denom = 1./denom;

    for( i = 0; i < src->rows; i++ )
    {
        float* dxdyp = (float*)(dxdy->data.ptr + i*dxdy->step);
        float* dx2p = (float*)(dx2->data.ptr + i*dx2->step);
        float* dy2p = (float*)(dy2->data.ptr + i*dy2->step);

        for( j = 0; j < src->cols; j++ )
        {
            double xval = dx2p[j], yval = dy2p[j];
            dxdyp[j] = (float)(xval*yval*denom);
            dx2p[j] = (float)(xval*xval*denom);
            dy2p[j] = (float)(yval*yval*denom);
        }
    }

    CV_CALL( src2 = cvCreateMat( src->rows + block_size - 1, src->cols + block_size - 1, CV_32F ));
    CV_CALL( kernel = cvCreateMat( block_size, block_size, CV_32F ));
    cvTsAdd( 0, cvScalarAll(0), 0, cvScalarAll(0), cvScalarAll(1./*(block_size*block_size)*/), kernel, 0 );
    anchor = cvPoint( block_size/2, block_size/2 );
    
    cvTsPrepareToFilter( dx2, src2, anchor, CV_TS_BORDER_REPLICATE );
    cvTsConvolve2D( src2, dx2, kernel, anchor );
    cvTsPrepareToFilter( dy2, src2, anchor, CV_TS_BORDER_REPLICATE );
    cvTsConvolve2D( src2, dy2, kernel, anchor );
    cvTsPrepareToFilter( dxdy, src2, anchor, CV_TS_BORDER_REPLICATE );
    cvTsConvolve2D( src2, dxdy, kernel, anchor );

    if( mode == 0 )
    {
        for( i = 0; i < src->rows; i++ )
        {
            float* eigenvp = (float*)(eigenv->data.ptr + i*eigenv->step);
            const float* dxdyp = (float*)(dxdy->data.ptr + i*dxdy->step);
            const float* dx2p = (float*)(dx2->data.ptr + i*dx2->step);
            const float* dy2p = (float*)(dy2->data.ptr + i*dy2->step);

            for( j = 0; j < src->cols; j++ )
            {
                double a = dx2p[j], b = dxdyp[j], c = dy2p[j];
                double d = sqrt((a-c)*(a-c) + 4*b*b);
                double l1 = 0.5*(a + c + d);
                double l2 = 0.5*(a + c - d);
                double x1, y1, x2, y2, s;
                
                if( fabs(a - l1) + fabs(b) >= 1e-3 )
                    x1 = b, y1 = l1 - a;
                else
                    x1 = l1 - c, y1 = b;
                s = 1./(sqrt(x1*x1+y1*y1)+DBL_EPSILON);
                x1 *= s; y1 *= s;

                if( fabs(a - l2) + fabs(b) >= 1e-3 )
                    x2 = b, y2 = l2 - a;
                else
                    x2 = l2 - c, y2 = b;
                s = 1./(sqrt(x2*x2+y2*y2)+DBL_EPSILON);
                x2 *= s; y2 *= s;

                eigenvp[j*6] = (float)l1;
                eigenvp[j*6+1] = (float)l2;
                eigenvp[j*6+2] = (float)x1;
                eigenvp[j*6+3] = (float)y1;
                eigenvp[j*6+4] = (float)x2;
                eigenvp[j*6+5] = (float)y2;
            }
        }
    }
    else if( mode == 1 )
    {
        for( i = 0; i < src->rows; i++ )
        {
            float* eigenvp = (float*)(eigenv->data.ptr + i*eigenv->step);
            const float* dxdyp = (float*)(dxdy->data.ptr + i*dxdy->step);
            const float* dx2p = (float*)(dx2->data.ptr + i*dx2->step);
            const float* dy2p = (float*)(dy2->data.ptr + i*dy2->step);

            for( j = 0; j < src->cols; j++ )
            {
                double a = dx2p[j], b = dxdyp[j], c = dy2p[j];
                double d = sqrt((a-c)*(a-c) + 4*b*b);
                eigenvp[j] = (float)(0.5*(a + c - d));
            }
        }
    }

    __END__;

    cvReleaseMat( &dx2 );
    cvReleaseMat( &dy2 );
    cvReleaseMat( &dxdy );
    cvReleaseMat( &src2 );
    cvReleaseMat( &kernel );
}


// min eigenval
class CV_MinEigenValTest : public CV_FeatureSelBaseTest
{
public:
    CV_MinEigenValTest();

protected:
    void run_func();
    void prepare_to_validation( int );
};


CV_MinEigenValTest::CV_MinEigenValTest()
    : CV_FeatureSelBaseTest( "features-mineval", "cvCornerMinEigenVal", 1 )
{
}


void CV_MinEigenValTest::run_func()
{
    cvCornerMinEigenVal( test_array[INPUT][0], test_array[OUTPUT][0],
                         block_size, aperture_size );
}


void CV_MinEigenValTest::prepare_to_validation( int /*test_case_idx*/ )
{
    cvTsCornerEigenValsVecs( &test_mat[INPUT][0], &test_mat[REF_OUTPUT][0],
                             block_size, aperture_size, 1 );
}


CV_MinEigenValTest features_mineval;


// eigenval's & vec's
class CV_EigenValVecTest : public CV_FeatureSelBaseTest
{
public:
    CV_EigenValVecTest();

protected:
    void run_func();
    void prepare_to_validation( int );
};


CV_EigenValVecTest::CV_EigenValVecTest()
    : CV_FeatureSelBaseTest( "features-evalvec", "cvCornerEigenValsAndVecs", 6 )
{
}


void CV_EigenValVecTest::run_func()
{
    cvCornerEigenValsAndVecs( test_array[INPUT][0], test_array[OUTPUT][0],
                              block_size, aperture_size );
}


void CV_EigenValVecTest::prepare_to_validation( int /*test_case_idx*/ )
{
    cvTsCornerEigenValsVecs( &test_mat[INPUT][0], &test_mat[REF_OUTPUT][0],
                             block_size, aperture_size, 0 );
}


CV_EigenValVecTest features_evalvec;


// precornerdetect
class CV_PreCornerDetectTest : public CV_FeatureSelBaseTest
{
public:
    CV_PreCornerDetectTest();

protected:
    void run_func();
    void prepare_to_validation( int );
    int prepare_test_case( int );
};


CV_PreCornerDetectTest::CV_PreCornerDetectTest()
    : CV_FeatureSelBaseTest( "features-precorner", "cvPreCornerDetect", 1 )
{
}


void CV_PreCornerDetectTest::run_func()
{
    cvPreCornerDetect( test_array[INPUT][0], test_array[OUTPUT][0], aperture_size );
}


int CV_PreCornerDetectTest::prepare_test_case( int test_case_idx )
{
    int code = CV_FeatureSelBaseTest::prepare_test_case( test_case_idx );
    if( aperture_size < 0 )
        aperture_size = 3;
    return code;
}


void CV_PreCornerDetectTest::prepare_to_validation( int /*test_case_idx*/ )
{
    /*cvTsCornerEigenValsVecs( &test_mat[INPUT][0], &test_mat[REF_OUTPUT][0],
                             block_size, aperture_size, 0 );*/
    const CvMat* src = &test_mat[INPUT][0];
    CvMat* dst = &test_mat[REF_OUTPUT][0];

    int type = CV_MAT_TYPE(src->type), ftype = CV_32FC1;
    CvPoint anchor = { aperture_size/2, aperture_size/2 };
    double denom;
    int i, j;

    CvMat* dx = cvCreateMat( src->rows, src->cols, ftype );
    CvMat* dy = cvCreateMat( src->rows, src->cols, ftype );
    CvMat* d2x = cvCreateMat( src->rows, src->cols, ftype );
    CvMat* d2y = cvCreateMat( src->rows, src->cols, ftype );
    CvMat* dxy = cvCreateMat( src->rows, src->cols, ftype );
    CvMat* tmp = cvCreateMat( src->rows + aperture_size - 1,
                        src->cols + aperture_size - 1, ftype );
    CvMat* kernel = cvCreateMat( aperture_size, aperture_size, ftype );

    if( type != ftype )
    {
        cvTsAdd( src, cvScalarAll(1./255), 0, cvScalarAll(0.), cvScalarAll(0.), dx, 0 );
        src = dx;
    }

    cvTsPrepareToFilter( src, tmp, anchor, CV_TS_BORDER_REPLICATE );

    cvTsCalcSobelKernel2D( 1, 0, aperture_size, 0, kernel );
    cvTsConvolve2D( tmp, dx, kernel, anchor );
    cvTsCalcSobelKernel2D( 0, 1, aperture_size, 0, kernel );
    cvTsConvolve2D( tmp, dy, kernel, anchor );
    cvTsCalcSobelKernel2D( 2, 0, aperture_size, 0, kernel );
    cvTsConvolve2D( tmp, d2x, kernel, anchor );
    cvTsCalcSobelKernel2D( 0, 2, aperture_size, 0, kernel );
    cvTsConvolve2D( tmp, d2y, kernel, anchor );
    cvTsCalcSobelKernel2D( 1, 1, aperture_size, 0, kernel );
    cvTsConvolve2D( tmp, dxy, kernel, anchor );

    denom = 1 << (aperture_size-1);
    denom = denom * denom * denom;
    denom = 1./denom;

    for( i = 0; i < src->rows; i++ )
    {
        const float* _dx = (const float*)(dx->data.ptr + i*dx->step);
        const float* _dy = (const float*)(dy->data.ptr + i*dy->step);
        const float* _d2x = (const float*)(d2x->data.ptr + i*d2x->step);
        const float* _d2y = (const float*)(d2y->data.ptr + i*d2y->step);
        const float* _dxy = (const float*)(dxy->data.ptr + i*dxy->step);
        float* corner = (float*)(dst->data.ptr + i*dst->step);

        for( j = 0; j < src->cols; j++ )
        {
            double x = _dx[j];
            double y = _dy[j];

            corner[j] = (float)(denom*(x*x*_d2y[j] + y*y*_d2x[j] - 2*x*y*_dxy[j]));
        }
    }

    cvReleaseMat( &dx );
    cvReleaseMat( &dy );
    cvReleaseMat( &d2x );
    cvReleaseMat( &d2y );
    cvReleaseMat( &dxy );
    cvReleaseMat( &tmp );
    cvReleaseMat( &kernel );
}


CV_PreCornerDetectTest precorner;

