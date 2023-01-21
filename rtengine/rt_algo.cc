/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2017-2018 Ingo Weyrich <heckflosse67@gmx.de>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <complex.h>
#include <fftw3.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "gauss.h"
#include "opthelper.h"
#include "rt_algo.h"
#include "rt_math.h"
#include "sleef.h"
#include "../rtgui/threadutils.h"
#include "imagefloat.h"

namespace {

float calcBlendFactor(float val, float threshold) {
    // sigmoid function
    // result is in ]0;1] range
    // inflexion point is at (x, y) (threshold, 0.5)
    return 1.f / (1.f + xexpf(16.f - 16.f * val / threshold));
}

#ifdef __SSE2__
vfloat calcBlendFactor(vfloat valv, vfloat thresholdv) {
    // sigmoid function
    // result is in ]0;1] range
    // inflexion point is at (x, y) (threshold, 0.5)
    const vfloat onev = F2V(1.f);
    const vfloat c16v = F2V(16.f);
    return onev / (onev + xexpf(c16v - c16v * valv / thresholdv));
}
#endif

float tileAverage(float **data, size_t tileY, size_t tileX, size_t tilesize) {

    float avg = 0.f;
#ifdef __SSE2__
    vfloat avgv = ZEROV;
#endif
    for (std::size_t y = tileY; y < tileY + tilesize; ++y) {
        std::size_t x = tileX;
#ifdef __SSE2__
        for (; x < tileX + tilesize - 3; x += 4) {
            avgv += LVFU(data[y][x]);
        }
#endif
        for (; x < tileX + tilesize; ++x) {
            avg += data[y][x];
        }
    }
#ifdef __SSE2__
    avg += vhadd(avgv);
#endif
    return avg / rtengine::SQR(tilesize);
}

float tileVariance(float **data, size_t tileY, size_t tileX, size_t tilesize, float avg) {

    float var = 0.f;
#ifdef __SSE2__
    vfloat varv = ZEROV;
    const vfloat avgv = F2V(avg);
#endif
    for (std::size_t y = tileY; y < tileY + tilesize; ++y) {
        std::size_t x = tileX;
#ifdef __SSE2__
        for (; x < tileX + tilesize - 3; x += 4) {
            varv += SQRV(LVFU(data[y][x]) - avgv);
        }
#endif
        for (; x < tileX + tilesize; ++x) {
            var += rtengine::SQR(data[y][x] - avg);
        }
    }
#ifdef __SSE2__
    var += vhadd(varv);
#endif
    return var / (rtengine::SQR(tilesize) * avg);
}

float calcContrastThreshold(float** luminance, int tileY, int tileX, int tilesize, float factor) {

    const float scale = 0.0625f / 327.68f * factor;
    std::vector<std::vector<float>> blend(tilesize - 4, std::vector<float>(tilesize - 4));

#ifdef __SSE2__
    const vfloat scalev = F2V(scale);
#endif

    for(int j = tileY + 2; j < tileY + tilesize - 2; ++j) {
        int i = tileX + 2;
#ifdef __SSE2__
        for(; i < tileX + tilesize - 5; i += 4) {
            vfloat contrastv = vsqrtf(SQRV(LVFU(luminance[j][i+1]) - LVFU(luminance[j][i-1])) + SQRV(LVFU(luminance[j+1][i]) - LVFU(luminance[j-1][i])) +
                                      SQRV(LVFU(luminance[j][i+2]) - LVFU(luminance[j][i-2])) + SQRV(LVFU(luminance[j+2][i]) - LVFU(luminance[j-2][i]))) * scalev;
            STVFU(blend[j - tileY - 2][i - tileX - 2], contrastv);
        }
#endif
        for(; i < tileX + tilesize - 2; ++i) {

            float contrast = sqrtf(rtengine::SQR(luminance[j][i+1] - luminance[j][i-1]) + rtengine::SQR(luminance[j+1][i] - luminance[j-1][i]) + 
                                   rtengine::SQR(luminance[j][i+2] - luminance[j][i-2]) + rtengine::SQR(luminance[j+2][i] - luminance[j-2][i])) * scale;

            blend[j - tileY - 2][i - tileX - 2] = contrast;
        }
    }

    const float limit = rtengine::SQR(tilesize - 4) / 100.f;

    int c;
    for (c = 1; c < 100; ++c) {
        const float contrastThreshold = c / 100.f;
        float sum = 0.f;
#ifdef __SSE2__
        const vfloat contrastThresholdv = F2V(contrastThreshold);
        vfloat sumv = ZEROV;
#endif

        for(int j = 0; j < tilesize - 4; ++j) {
            int i = 0;
#ifdef __SSE2__
            for(; i < tilesize - 7; i += 4) {
                sumv += calcBlendFactor(LVFU(blend[j][i]), contrastThresholdv);
            }
#endif
            for(; i < tilesize - 4; ++i) {
                sum += calcBlendFactor(blend[j][i], contrastThreshold);
            }
        }
#ifdef __SSE2__
        sum += vhadd(sumv);
#endif
        if (sum <= limit) {
            break;
        }
    }

    return c / 100.f;
}

} // namespace

namespace rtengine {

extern MyMutex *fftwMutex;


void findMinMaxPercentile(const float* data, size_t size, float minPrct, float& minOut, float maxPrct, float& maxOut, bool multithread)
{
    // Copyright (c) 2017 Ingo Weyrich <heckflosse67@gmx.de>
    // We need to find the (minPrct*size) smallest value and the (maxPrct*size) smallest value in data.
    // We use a histogram based search for speed and to reduce memory usage.
    // Memory usage of this method is histoSize * sizeof(uint32_t) * (t + 1) byte,
    // where t is the number of threads and histoSize is in [1;65536].
    // Processing time is O(n) where n is size of the input array.
    // It scales well with multiple threads if the size of the input array is large.
    // The current implementation is not guaranteed to work correctly if size > 2^32 (4294967296).

    assert(minPrct <= maxPrct);

    if (size == 0) {
        return;
    }

    size_t numThreads = 1;
#ifdef _OPENMP
    // Because we have an overhead in the critical region of the main loop for each thread
    // we make a rough calculation to reduce the number of threads for small data size.
    // This also works fine for the minmax loop.
    if (multithread) {
        const size_t maxThreads = omp_get_num_procs();
        while (size > numThreads * numThreads * 16384 && numThreads < maxThreads) {
            ++numThreads;
        }
    }
#endif

    // We need min and max value of data to calculate the scale factor for the histogram
    float minVal = data[0];
    float maxVal = data[0];
#ifdef _OPENMP
    #pragma omp parallel for reduction(min:minVal) reduction(max:maxVal) num_threads(numThreads)
#endif
    for (size_t i = 1; i < size; ++i) {
        minVal = std::min(minVal, data[i]);
        maxVal = std::max(maxVal, data[i]);
    }

    if (std::fabs(maxVal - minVal) == 0.f) { // fast exit, also avoids division by zero in calculation of scale factor
        minOut = maxOut = minVal;
        return;
    }

    // Caution: Currently this works correctly only for histoSize in range[1;65536].
    // For small data size (i.e. thumbnails) we reduce the size of the histogram to the size of data.
    const unsigned int histoSize = std::min<size_t>(65536, size);

    // calculate scale factor to use full range of histogram
    const float scale = (histoSize - 1) / (maxVal - minVal);

    // We need one main histogram
    std::vector<uint32_t> histo(histoSize, 0);

    if (numThreads == 1) {
        // just one thread => use main histogram
        for (size_t i = 0; i < size; ++i) {
            // we have to subtract minVal and multiply with scale to get the data in [0;histosize] range
            histo[static_cast<uint16_t>(scale * (data[i] - minVal))]++;
        }
    } else {
#ifdef _OPENMP
    #pragma omp parallel num_threads(numThreads)
#endif
        {
            // We need one histogram per thread
            std::vector<uint32_t> histothr(histoSize, 0);

#ifdef _OPENMP
            #pragma omp for nowait
#endif
            for (size_t i = 0; i < size; ++i) {
                // we have to subtract minVal and multiply with scale to get the data in [0;histosize] range
                histothr[static_cast<uint16_t>(scale * (data[i] - minVal))]++;
            }

#ifdef _OPENMP
            #pragma omp critical
#endif
            {
                // add per thread histogram to main histogram
#ifdef _OPENMP
                #pragma omp simd
#endif

                for (size_t i = 0; i < histoSize; ++i) {
                    histo[i] += histothr[i];
                }
            }
        }
    }

    size_t k = 0;
    size_t count = 0;

    // find (minPrct*size) smallest value
    const float threshmin = minPrct * size;
    while (count < threshmin) {
        count += histo[k++];
    }

    if (k > 0) { // interpolate
        const size_t count_ = count - histo[k - 1];
        const float c0 = count - threshmin;
        const float c1 = threshmin - count_;
        minOut = (c1 * k + c0 * (k - 1)) / (c0 + c1);
    } else {
        minOut = k;
    }
    // go back to original range
    minOut /= scale;
    minOut += minVal;
    minOut = rtengine::LIM(minOut, minVal, maxVal);

    // find (maxPrct*size) smallest value
    const float threshmax = maxPrct * size;
    while (count < threshmax) {
        count += histo[k++];
    }

    if (k > 0) { // interpolate
        const size_t count_ = count - histo[k - 1];
        const float c0 = count - threshmax;
        const float c1 = threshmax - count_;
        maxOut = (c1 * k + c0 * (k - 1)) / (c0 + c1);
    } else {
        maxOut = k;
    }
    // go back to original range
    maxOut /= scale;
    maxOut += minVal;
    maxOut = rtengine::LIM(maxOut, minVal, maxVal);
}

void buildBlendMask(float** luminance, float **blend, int W, int H, float &contrastThreshold, float amount, bool autoContrast, float blur_radius, float luminance_factor)
{
    if (autoContrast) {
        const float minLuminance = 2000.f / luminance_factor;
        const float maxLuminance = 20000.f / luminance_factor;
        constexpr float minTileVariance = 0.5f;
        for (int pass = 0; pass < 2; ++pass) {
            const int tilesize = 80 / (pass + 1);
            const int skip = pass == 0 ? tilesize : tilesize / 4;
            const int numTilesW = W / skip - 3 * pass;
            const int numTilesH = H / skip - 3 * pass;
            std::vector<std::vector<float>> variances(numTilesH, std::vector<float>(numTilesW));

#ifdef _OPENMP
            #pragma omp parallel for schedule(dynamic)
#endif
            for (int i = 0; i < numTilesH; ++i) {
                const int tileY = i * skip;
                for (int j = 0; j < numTilesW; ++j) {
                    const int tileX = j * skip;
                    const float avg = tileAverage(luminance, tileY, tileX, tilesize);
                    if (avg < minLuminance || avg > maxLuminance) {
                        // too dark or too bright => skip the tile
                        variances[i][j] = RT_INFINITY_F;
                        continue;
                    } else {
                        variances[i][j] = tileVariance(luminance, tileY, tileX, tilesize, avg);
                        // exclude tiles with a variance less than minTileVariance
                        variances[i][j] = variances[i][j] < minTileVariance ? RT_INFINITY_F : variances[i][j];
                    }
                }
            }

            float minvar = RT_INFINITY_F;
            int minI = 0, minJ = 0;
            for (int i = 0; i < numTilesH; ++i) {
                for (int j = 0; j < numTilesW; ++j) {
                    if (variances[i][j] < minvar) {
                        minvar = variances[i][j];
                        minI = i;
                        minJ = j;
                    }
                }
            }

            if (minvar <= 1.f || pass == 1) {
                const int minY = skip * minI;
                const int minX = skip * minJ;
                if (pass == 0) {
                    // a variance <= 1 means we already found a flat region and can skip second pass
                    contrastThreshold = calcContrastThreshold(luminance, minY, minX, tilesize, luminance_factor);
                    break;
                } else {
                    // in second pass we allow a variance of 4
                    // we additionally scan the tiles +-skip pixels around the best tile from pass 2
                    // Means we scan (2 * skip + 1)^2 tiles in this step to get a better hit rate
                    // fortunately the scan is quite fast, so we use only one core and don't parallelize
                    const int topLeftYStart = std::max(minY - skip, 0);
                    const int topLeftXStart = std::max(minX - skip, 0);
                    const int topLeftYEnd = std::min(minY + skip, H - tilesize);
                    const int topLeftXEnd = std::min(minX + skip, W - tilesize);
                    const int numTilesH = topLeftYEnd - topLeftYStart + 1;
                    const int numTilesW = topLeftXEnd - topLeftXStart + 1;

                    std::vector<std::vector<float>> variances(numTilesH, std::vector<float>(numTilesW));
                    for (int i = 0; i < numTilesH; ++i) {
                        const int tileY = topLeftYStart + i;
                        for (int j = 0; j < numTilesW; ++j) {
                            const int tileX = topLeftXStart + j;
                            const float avg = tileAverage(luminance, tileY, tileX, tilesize);

                            if (avg < minLuminance || avg > maxLuminance) {
                                // too dark or too bright => skip the tile
                                variances[i][j] = RT_INFINITY_F;
                                continue;
                            } else {
                                variances[i][j] = tileVariance(luminance, tileY, tileX, tilesize, avg);
                            // exclude tiles with a variance less than minTileVariance
                            variances[i][j] = variances[i][j] < minTileVariance ? RT_INFINITY_F : variances[i][j];
                            }
                        }
                    }

                    float minvar = RT_INFINITY_F;
                    int minI = 0, minJ = 0;
                    for (int i = 0; i < numTilesH; ++i) {
                        for (int j = 0; j < numTilesW; ++j) {
                            if (variances[i][j] < minvar) {
                                minvar = variances[i][j];
                                minI = i;
                                minJ = j;
                            }
                        }
                    }

                    contrastThreshold = minvar <= 8.f ? calcContrastThreshold(luminance, topLeftYStart + minI, topLeftXStart + minJ, tilesize, luminance_factor) : 0.f;
                }
            }
        }
    }

    if(contrastThreshold == 0.f) {
        for(int j = 0; j < H; ++j) {
            for(int i = 0; i < W; ++i) {
                blend[j][i] = amount;
            }
        }
    } else {
        const float scale = 0.0625f / 327.68f * luminance_factor;
#ifdef _OPENMP
        #pragma omp parallel
#endif
        {
#ifdef __SSE2__
            const vfloat contrastThresholdv = F2V(contrastThreshold);
            const vfloat scalev = F2V(scale);
            const vfloat amountv = F2V(amount);
#endif
#ifdef _OPENMP
            #pragma omp for schedule(dynamic,16)
#endif

            for(int j = 2; j < H - 2; ++j) {
                int i = 2;
#ifdef __SSE2__
                for(; i < W - 5; i += 4) {
                    vfloat contrastv = vsqrtf(SQRV(LVFU(luminance[j][i+1]) - LVFU(luminance[j][i-1])) + SQRV(LVFU(luminance[j+1][i]) - LVFU(luminance[j-1][i])) +
                                              SQRV(LVFU(luminance[j][i+2]) - LVFU(luminance[j][i-2])) + SQRV(LVFU(luminance[j+2][i]) - LVFU(luminance[j-2][i]))) * scalev;

                    STVFU(blend[j][i], amountv * calcBlendFactor(contrastv, contrastThresholdv));
                }
#endif
                for(; i < W - 2; ++i) {

                    float contrast = sqrtf(rtengine::SQR(luminance[j][i+1] - luminance[j][i-1]) + rtengine::SQR(luminance[j+1][i] - luminance[j-1][i]) + 
                                           rtengine::SQR(luminance[j][i+2] - luminance[j][i-2]) + rtengine::SQR(luminance[j+2][i] - luminance[j-2][i])) * scale;

                    blend[j][i] = amount * calcBlendFactor(contrast, contrastThreshold);
                }
            }

#ifdef _OPENMP
            #pragma omp single
#endif
            {
                // upper border
                for(int j = 0; j < 2; ++j) {
                    for(int i = 2; i < W - 2; ++i) {
                        blend[j][i] = blend[2][i];
                    }
                }
                // lower border
                for(int j = H - 2; j < H; ++j) {
                    for(int i = 2; i < W - 2; ++i) {
                        blend[j][i] = blend[H-3][i];
                    }
                }
                for(int j = 0; j < H; ++j) {
                    // left border
                    blend[j][0] = blend[j][1] = blend[j][2];
                    // right border
                    blend[j][W - 2] = blend[j][W - 1] = blend[j][W - 3];
                }
            }

#ifdef __SSE2__
            // flush denormals to zero for gaussian blur to avoid performance penalty if there are a lot of zero values in the mask
            const auto oldMode = _MM_GET_FLUSH_ZERO_MODE();
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

            // blur blend mask to smooth transitions
            gaussianBlur(blend, blend, W, H, blur_radius); //2.0);

#ifdef __SSE2__
            _MM_SET_FLUSH_ZERO_MODE(oldMode);
#endif
        }
    }
}


void markImpulse(int width, int height, float **const src, char **impulse, float thresh)
{
    // buffer for the lowpass image
    float * lpf[height] ALIGNED16;
    lpf[0] = new float [width * height];

    for (int i = 1; i < height; i++) {
        lpf[i] = lpf[i - 1] + width;
    }

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        gaussianBlur(const_cast<float **>(src), lpf, width, height, max(2.f, thresh - 1.f));
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    float impthr = max(1.f, 5.5f - thresh);
    float impthrDiv24 = impthr / 24.0f;         //Issue 1671: moved the Division outside the loop, impthr can be optimized out too, but I let in the code at the moment


#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        int i1, j1, j;
        float hpfabs, hfnbrave;
#ifdef __SSE2__
        vfloat hfnbravev, hpfabsv;
        vfloat impthrDiv24v = F2V( impthrDiv24 );
#endif
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int i = 0; i < height; i++) {
            for (j = 0; j < 2; j++) {
                hpfabs = fabs(src[i][j] - lpf[i][j]);

                //block average of high pass data
                for (i1 = max(0, i - 2), hfnbrave = 0; i1 <= min(i + 2, height - 1); i1++ )
                    for (j1 = 0; j1 <= j + 2; j1++) {
                        hfnbrave += fabs(src[i1][j1] - lpf[i1][j1]);
                    }

                impulse[i][j] = (hpfabs > ((hfnbrave - hpfabs) * impthrDiv24));
            }

#ifdef __SSE2__

            for (; j < width - 5; j += 4) {
                hfnbravev = ZEROV;
                hpfabsv = vabsf(LVFU(src[i][j]) - LVFU(lpf[i][j]));

                //block average of high pass data
                for (i1 = max(0, i - 2); i1 <= min(i + 2, height - 1); i1++ ) {
                    for (j1 = j - 2; j1 <= j + 2; j1++) {
                        hfnbravev += vabsf(LVFU(src[i1][j1]) - LVFU(lpf[i1][j1]));
                    }
                }

                int mask = _mm_movemask_ps((hfnbravev - hpfabsv) * impthrDiv24v - hpfabsv);
                impulse[i][j] = (mask & 1);
                impulse[i][j + 1] = ((mask & 2) >> 1);
                impulse[i][j + 2] = ((mask & 4) >> 2);
                impulse[i][j + 3] = ((mask & 8) >> 3);
            }

#endif

            for (; j < width - 2; j++) {
                hpfabs = fabs(src[i][j] - lpf[i][j]);

                //block average of high pass data
                for (i1 = max(0, i - 2), hfnbrave = 0; i1 <= min(i + 2, height - 1); i1++ )
                    for (j1 = j - 2; j1 <= j + 2; j1++) {
                        hfnbrave += fabs(src[i1][j1] - lpf[i1][j1]);
                    }

                impulse[i][j] = (hpfabs > ((hfnbrave - hpfabs) * impthrDiv24));
            }

            for (; j < width; j++) {
                hpfabs = fabs(src[i][j] - lpf[i][j]);

                //block average of high pass data
                for (i1 = max(0, i - 2), hfnbrave = 0; i1 <= min(i + 2, height - 1); i1++ )
                    for (j1 = j - 2; j1 < width; j1++) {
                        hfnbrave += fabs(src[i1][j1] - lpf[i1][j1]);
                    }

                impulse[i][j] = (hpfabs > ((hfnbrave - hpfabs) * impthrDiv24));
            }
        }
    }

    delete [] lpf[0];
}

// Code adapted from Blender's project
// https://developer.blender.org/diffusion/B/browse/master/source/blender/blenlib/intern/math_geom.c;3b4a8f1cfa7339f3db9ddd4a7974b8cc30d7ff0b$2411
float polyFill(float **buffer, int width, int height, const std::vector<CoordD> &poly, const float color)
{

    // First point of the polygon in image space
    int xStart = int(poly[0].x + 0.5);
    int yStart = int(poly[0].y + 0.5);
    int xEnd = xStart;
    int yEnd = yStart;

    // Find boundaries
    for (auto point : poly) {

        // X bounds
        if (int(point.x) < xStart) {
            xStart = int(point.x);
        } else if (int(point.x) > xEnd) {
            xEnd = int(point.x);
        }

        // Y bounds
        if (int(point.y) < yStart) {
            yStart = int(point.y);
        } else if (int(point.y) > yEnd) {
            yEnd = int(point.y);
        }
    }

    float ret = rtengine::min<int>(xEnd - xStart, yEnd - yStart);

    xStart = rtengine::LIM<int>(xStart, 0., width - 1);
    xEnd = rtengine::LIM<int>(xEnd, xStart, width - 1);
    yStart = rtengine::LIM<int>(yStart, 0., height - 1);
    yEnd = rtengine::LIM<int>(yEnd, yStart, height - 1);
    
    std::vector<int> nodeX;

    //  Loop through the rows of the image.
    for (int y = yStart; y <= yEnd; ++y) {
        nodeX.clear();

        //  Build a list of nodes.
        size_t j = poly.size() - 1;
        for (size_t i = 0; i < poly.size(); ++i) {
            if ((poly[i].y < double(y) && poly[j].y >= double(y))
            ||  (poly[j].y < double(y) && poly[i].y >= double(y)))
            {
                //TODO: Check rounding here ?
                // Possibility to add antialiasing here by calculating the distance of the value from the middle of the pixel (0.5)
                nodeX.push_back(int(poly[i].x + (double(y) - poly[i].y) / (poly[j].y - poly[i].y) * (poly[j].x - poly[i].x)));
            }
            j = i;
        }

        //  Sort the nodes
        std::sort(nodeX.begin(), nodeX.end());

        //  Fill the pixels between node pairs.
        for (size_t i = 0; i < nodeX.size(); i += 2) {
            if (nodeX.at(i) > xEnd) break;
            if (nodeX.at(i + 1) > xStart ) {
                if (nodeX.at(i) < xStart ) {
                    nodeX.at(i) = xStart;
                }
                if (nodeX.at(i + 1) > xEnd) {
                    nodeX.at(i + 1) = xEnd;
                }
                for (int x = nodeX.at(i); x <= nodeX.at(i + 1); ++x) {
                    buffer[y][x] = color;
                }
            }
        }
    }

    return ret;//float(rtengine::min<int>(xEnd - xStart, yEnd - yStart));
}


namespace {

inline int round_up_pow2(int dim)
{
    // from https://graphics.stanford.edu/~seander/bithacks.html
    assert (dim > 0);
    unsigned int v = dim;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline int find_fast_dim(int dim)
{
    // as per the FFTW docs:
    //
    //   FFTW is generally best at handling sizes of the form
    //     2^a 3^b 5^c 7^d 11^e 13^f,
    //   where e+f is either 0 or 1.
    //
    // Here, we try to round up to the nearest dim that can be expressed in
    // the above form. This is not exhaustive, but should be ok for pictures
    // up to 100MPix at least

    int d1 = round_up_pow2 (dim);
    std::vector<int> d = {
        d1 / 128 * 65,
        d1 / 64 * 33,
        d1 / 512 * 273,
        d1 / 16 * 9,
        d1 / 8 * 5,
        d1 / 16 * 11,
        d1 / 128 * 91,
        d1 / 4 * 3,
        d1 / 64 * 49,
        d1 / 16 * 13,
        d1 / 8 * 7,
        d1
    };

    for (size_t i = 0; i < d.size(); ++i) {
        if (d[i] >= dim) {
            return d[i];
        }
    }

    assert(false);
    return dim;
}


void do_convolution(fftwf_plan fwd_plan, fftwf_plan inv_plan, fftwf_complex *kernel_fft, int kernel_radius, int pH, int pW, float *buf, fftwf_complex *buf_fft, int W, int H, float **const src, float **dst, bool multithread)
{
#ifdef _OPENMP
#   pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < pH; ++y) {
        int yy = LIM(y - kernel_radius, 0, H-1);
        for (int x = 0; x < pW; ++x) {
            int xx = LIM(x - kernel_radius, 0, W-1);
            buf[y * pW + x] = src[yy][xx];
        }
    }

    fftwf_execute(fwd_plan);

#ifdef _OPENMP
#   pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < pH; ++y) {
        for (int x = 0, end = pW / 2 + 1; x < end; ++x) {
            const int i = y * end + x;
            float p = buf_fft[i][0], q = buf_fft[i][1];
            float r = kernel_fft[i][0], s = kernel_fft[i][1];
            buf_fft[i][0] = p * r - q * s;
            buf_fft[i][1] = p * s + q * r;
        }
    }

    fftwf_execute(inv_plan);

    const int K = 2 * kernel_radius;
    const float norm = pH * pW;
#ifdef _OPENMP
#           pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = (y + K) * pW + x + K;
            assert(idx < pH * pW);
            dst[y][x] = buf[idx] / norm;
        }
    }
}


fftwf_complex *prepare_kernel(const array2D<float> &kernel, float *buf, int pW, int pH, bool multithread)
{
    fftwf_complex *kernel_fft = fftwf_alloc_complex(pH * (pW / 2 + 1));
    const int K = kernel.width();

#ifdef _OPENMP
#       pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < pH; ++y) {
        for (int x = 0; x < pW; ++x) {
            if (y < K && x < K) {
                buf[y * pW + x] = kernel[y][x];
            } else {
                buf[y * pW + x] = 0.f;
            }
        }
    }

    auto plan = fftwf_plan_dft_r2c_2d(pH, pW, buf, kernel_fft, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);

    return kernel_fft;
}


struct ConvolutionData {
    int K;
    int W;
    int H;
    int pW;
    int pH;
    fftwf_complex *kernel_fft;
    float *buf;
    fftwf_complex *buf_fft;
    fftwf_plan fwd_plan;
    fftwf_plan inv_plan;
    bool multithread;

    ConvolutionData(const array2D<float> &kernel, int W, int H, bool multithread):
        K(0),
        kernel_fft(nullptr),
        buf(nullptr),
        buf_fft(nullptr),
        fwd_plan(nullptr),
        inv_plan(nullptr),
        multithread(multithread)
    {
        K = kernel.width();
        if (K == kernel.height()) {
            MyMutex::MyLock lock(*fftwMutex);

#ifdef RT_FFTW3F_OMP
            if (multithread) {
                fftwf_init_threads();
                fftwf_plan_with_nthreads(omp_get_num_procs());
            }
#endif
            
            this->W = W;
            this->H = H;
            pW = find_fast_dim(W + K);
            pH = find_fast_dim(H + K);

            buf = static_cast<float *>(fftwf_malloc(sizeof(float) * pH * pW));
            buf_fft = fftwf_alloc_complex(pH * (pW / 2 + 1));
            kernel_fft = prepare_kernel(kernel, buf, pW, pH, false);

            fwd_plan = fftwf_plan_dft_r2c_2d(pH, pW, buf, buf_fft, FFTW_ESTIMATE);
            inv_plan = fftwf_plan_dft_c2r_2d(pH, pW, buf_fft, buf, FFTW_ESTIMATE);
        }
    }

    ~ConvolutionData()
    {
        if (inv_plan) {
            fftwf_destroy_plan(inv_plan);
        }
        if (fwd_plan) {
            fftwf_destroy_plan(fwd_plan);
        }
        if (kernel_fft) {
            fftwf_free(kernel_fft);
        }
        if (buf_fft) {
            fftwf_free(buf_fft);
        }
        if (buf) {
            fftwf_free(buf);
        }
    }
};

} // namespace


Convolution::Convolution(const array2D<float> &kernel, int W, int H, bool multithread):
    data_(nullptr)
{
    data_ = new ConvolutionData(kernel, W, H, multithread);
}


Convolution::~Convolution()
{
    delete static_cast<ConvolutionData *>(data_);
}


void Convolution::operator()(float **src, float **dst)
{
    ConvolutionData *d = static_cast<ConvolutionData *>(data_);
    MyMutex::MyLock lock(*fftwMutex);

    do_convolution(d->fwd_plan, d->inv_plan, d->kernel_fft, d->K/2, d->pH, d->pW, d->buf, d->buf_fft, d->W, d->H, src, dst, d->multithread);
}


void Convolution::operator()(const array2D<float> &src, array2D<float> &dst)
{
    operator()(static_cast<float **>(const_cast<array2D<float> &>(src)), static_cast<float **>(dst));
}


void build_gaussian_kernel(float sigma, array2D<float> &res)
{
    static constexpr float threshold = 0.005f;
    int sz = (int(std::floor(1 + 2 * std::sqrt(-2.f * SQR(sigma) * std::log(threshold)))) + 1) | 1;
    const float two_sigma2 = 2.f * SQR(sigma);
    const auto gauss =
        [two_sigma2](float x) -> float
        {
            return std::exp(-SQR(x)/two_sigma2);
        };
    const auto gauss_integral =
        [&](float a, float b) -> float
        {
            return ((b - a) / 6.f) * (gauss(a) + 4.f * gauss((a + b) / 2.f) + gauss(b));
        };
    std::vector<float> row(sz);
    const float halfsz = float(sz / 2);
    for (int i = 0; i < sz; ++i) {
        float x = float(i) - halfsz;
        float val = gauss_integral(x - 0.5f, x + 0.5f);
        row[i] = val;
    }
    res(sz, sz);
    double totd = 0.0;
    for (int i = 0; i < sz; ++i) {
        for (int j = 0; j < sz; ++j) {
            float val = row[i] * row[j];
            res[i][j] = val;
            totd += val;
        }
    }
    const float tot = totd;
    for (int i = 0; i < sz; ++i) {
        for (int j = 0; j < sz; ++j) {
            res[i][j] /= tot;
        }
    }
}


void get_luminance(const Imagefloat *src, array2D<float> &out, const float ws[3][3], bool multithread)
{
    const int W = src->getWidth();
    const int H = src->getHeight();
    out(W, H);
    
#ifdef _OPENMP
#   pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            out[y][x] = Color::rgbLuminance(src->r(y, x), src->g(y, x), src->b(y, x), ws);
        }
    }
}

void multiply(Imagefloat *img, const array2D<float> &num, const array2D<float> &den, bool multithread)
{
    const int W = img->getWidth();
    const int H = img->getHeight();
    
#ifdef _OPENMP
#   pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (den[y][x] > 0.f) {
                const float f = num[y][x] / den[y][x];
                img->r(y, x) *= f;
                img->g(y, x) *= f;
                img->b(y, x) *= f;
            }
        }
    }
}

} // namespace rtengine
