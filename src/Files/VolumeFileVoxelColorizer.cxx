
/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __VOLUME_FILE_VOXEL_COLORIZER_DECLARE__
#include "VolumeFileVoxelColorizer.h"
#undef __VOLUME_FILE_VOXEL_COLORIZER_DECLARE__

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "ElapsedTimer.h"
#include "GiftiLabel.h"
#include "GroupAndNameHierarchyItem.h"
#include "NodeAndVoxelColoring.h"
#include "VolumeFile.h"

#include <cmath>

using namespace caret;


    
/**
 * \class caret::VolumeFileVoxelColorizer 
 * \brief Delegate for coloring a volumes voxels.
 */

/**
 * Constructor.
 *
 * @param volumeFile
 *    Volume file on which this instance colors voxels.
 */
VolumeFileVoxelColorizer::VolumeFileVoxelColorizer(VolumeFile* volumeFile)
: CaretObject()
{
    CaretAssert(volumeFile);
    
    m_volumeFile = volumeFile;
    
    int64_t dimNumberOfComponents;
    m_volumeFile->getDimensions(m_dimI,
                                m_dimJ,
                                m_dimK,
                                m_mapCount,
                                dimNumberOfComponents);
    
    m_voxelCountPerMap = m_dimI * m_dimJ * m_dimK;
    m_mapRGBACount = m_voxelCountPerMap * 4;
    
    for (int64_t i = 0; i < m_mapCount; i++) {
        m_mapRGBA.push_back(new uint8_t[m_mapRGBACount]);
        m_mapColoringValid.push_back(false);
    }
}

/**
 * Destructor.
 */
VolumeFileVoxelColorizer::~VolumeFileVoxelColorizer()
{
    for (int64_t i = 0; i < m_mapCount; i++) {
        delete[] m_mapRGBA[i];
    }
    m_mapRGBA.clear();
}

/**
 * Assign voxel coloring for a map.
 *
 * @param mapIndex
 *     Index of map.
 * @param palette  
 *     Palette used for scalar color assignment.  May be NULL for data
 *     not mapped with a palette.
 * @param thresholdVolume
 *     Volume that contains thresholding (if NULL indicates no thresholding).
 * @param thresholdVolumeMapIndex
 *     Index of map in thresholding volume.
 */
void
VolumeFileVoxelColorizer::assignVoxelColorsForMap(const int32_t mapIndex,
                                                  const Palette* palette,
                                                  const VolumeFile* thresholdVolume,
                                                  const int32_t /*thresholdVolumeMapIndex*/)
{
    CaretAssertVectorIndex(m_mapRGBA, mapIndex);
    
    ElapsedTimer timer;
    timer.start();
    
    /*
     * Pointer to map's data 
     */
    const float* mapDataPointer = m_volumeFile->getFrame(mapIndex);
    
    /*
     * Get access to threshold data
     */
    //float* thresholdDataPointer = NULL;
    bool ignoreThresholding = true;
    if (thresholdVolume != NULL) {
        int64_t threshI, threshJ, threshK, threshMapCount, threshNumberOfComponents;
        thresholdVolume->getDimensions(threshI,
                                       threshJ,
                                       threshK,
                                       threshMapCount,
                                       threshNumberOfComponents);
        if ((threshI != m_dimI)
            || (threshJ != m_dimJ)
            || (threshK != m_dimK)) {
            CaretLogSevere("Threshold volume ("
                           + thresholdVolume->getFileNameNoPath()
                           + ") dimensions do not match "
                           + m_volumeFile->getFileNameNoPath());
        }
        else {
            /*
             * Can use same voxel counter per map since volumes are
             * identical dimensions;
             */
            //thresholdDataPointer = thresholdVolume->m_data + m_voxelCountPerMap;//TSC: this is unused, and can easily be an invalid pointer - commenting out for unused warning
            ignoreThresholding = false;
        }
    }
    
    switch (m_volumeFile->getType()) {
        case SubvolumeAttributes::UNKNOWN:
        case SubvolumeAttributes::ANATOMY:
        case SubvolumeAttributes::FUNCTIONAL:
        {
            CaretAssert(palette);

            
            FastStatistics* statistics = NULL;
            switch (m_volumeFile->getPaletteNormalizationMode()) {
                case PaletteNormalizationModeEnum::NORMALIZATION_ALL_MAP_DATA:
                    statistics = const_cast<FastStatistics*>(m_volumeFile->getFileFastStatistics());
                    break;
                case PaletteNormalizationModeEnum::NORMALIZATION_SELECTED_MAP_DATA:
                    statistics = const_cast<FastStatistics*>(m_volumeFile->getMapFastStatistics(mapIndex));
                    break;
            }
            CaretAssert(statistics);
            

            NodeAndVoxelColoring::colorScalarsWithPalette(statistics, //m_volumeFile->getMapFastStatistics(mapIndex),
                                                          m_volumeFile->getMapPaletteColorMapping(mapIndex),
                                                          palette,
                                                          mapDataPointer,
                                                          mapDataPointer,
                                                          m_voxelCountPerMap,
                                                          m_mapRGBA[mapIndex],
                                                          ignoreThresholding);
            m_mapColoringValid[mapIndex] = true;
        }
            break;
        case SubvolumeAttributes::LABEL:
            if (m_voxelCountPerMap > 0) {
                NodeAndVoxelColoring::colorIndicesWithLabelTable(m_volumeFile->getMapLabelTable(mapIndex),
                                                                 &mapDataPointer[0],
                                                                 m_voxelCountPerMap,
                                                                 m_mapRGBA[mapIndex]);
                m_mapColoringValid[mapIndex] = true;
            }
            break;
        case SubvolumeAttributes::RGB:
        {
            const int32_t numberOfComponents = m_volumeFile->getNumberOfComponents();
            if ((numberOfComponents == 3)
                || (numberOfComponents == 4)) {
                const float* alphaComponents = ((numberOfComponents == 4)
                                                ? m_volumeFile->getFrame(mapIndex, 3)
                                                : NULL);
                
                NodeAndVoxelColoring::colorScalarsWithRGBA(m_volumeFile->getFrame(mapIndex, 0),
                                                           m_volumeFile->getFrame(mapIndex, 1),
                                                           m_volumeFile->getFrame(mapIndex, 2),
                                                           alphaComponents,
                                                           m_voxelCountPerMap,
                                                           m_mapRGBA[mapIndex]);
                m_mapColoringValid[mapIndex] = true;
            }
            else {
                CaretLogSevere("An RGB/RGBA volume must contain 3 or 4 components per voxel: "
                               + m_volumeFile->getFileNameNoPath());
            }
        }
            break;
        case SubvolumeAttributes::SEGMENTATION:
            break;
        case SubvolumeAttributes::VECTOR:
            break;
    }
    
    CaretLogFine("Time to color map named \""
                   + m_volumeFile->getMapName(mapIndex)
                   + " in volume file "
                   + m_volumeFile->getFileNameNoPath()
                   + " was "
                   + AString::number(timer.getElapsedTimeMilliseconds())
                   + " milliseconds");
}

/**
 * Invalidate the RGBA coloring for all maps.
 */
void
VolumeFileVoxelColorizer::invalidateColoring()
{
    std::fill(m_mapColoringValid.begin(),
              m_mapColoringValid.end(),
              false);
}

/**
 * Get voxel coloring for a slice in a map.  If voxel coloring is not ready
 * (it may be running in a different thread) this method will wait until the 
 * coloring is valid prior to returning the slice's coloring.
 *
 * @param mapIndex
 *     Index of map.
 * @param slicePlane
 *    Plane of the slice.
 * @param sliceIndex
 *    Index of the slice.
 * @param displayGroup
 *    The selected display group.
 * @param tabIndex
 *    Index of selected tab.
 * @param rgbaOut
 *    RGBA color components out.
 * @return
 *    Number of voxels with alpha greater than zero
 */
int64_t
VolumeFileVoxelColorizer::getVoxelColorsForSliceInMap(const int32_t mapIndex,
                                                      const VolumeSliceViewPlaneEnum::Enum slicePlane,
                                                      const int64_t sliceIndex,
                                                      const DisplayGroupEnum::Enum displayGroup,
                                                      const int32_t tabIndex,
                                                      uint8_t* rgbaOut) const
{
    CaretAssertVectorIndex(m_mapRGBA, mapIndex);
    CaretAssert(sliceIndex >= 0);
    CaretAssert(rgbaOut);
    
    int64_t iStart = 0;
    int64_t iEnd   = m_dimI - 1;
    int64_t jStart = 0;
    int64_t jEnd   = m_dimJ - 1;
    int64_t kStart = 0;
    int64_t kEnd   = m_dimK - 1;
    switch (slicePlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            kStart = sliceIndex;
            kEnd   = sliceIndex;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            jStart = sliceIndex;
            jEnd   = sliceIndex;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            iStart = sliceIndex;
            iEnd   = sliceIndex;
            break;
    }

    /*
     * Pointer to maps RGBA values
     */
    const uint8_t* mapRGBA = m_mapRGBA[mapIndex];
    
    const GiftiLabelTable* labelTable = (m_volumeFile->isMappedWithLabelTable()
                                         ? m_volumeFile->getMapLabelTable(mapIndex)
                                         : NULL);
    if (m_volumeFile->isMappedWithLabelTable()) {
        CaretAssert(labelTable);
    }
    
    int64_t validVoxelCount = 0;
    
    /*
     * Output RGBA values for slice
     */
    int64_t rgbaOutIndex = 0;
    for (int64_t k = kStart; k <= kEnd; k++) {
        for (int64_t j = jStart; j <= jEnd; j++) {
            for (int64_t i = iStart; i <= iEnd; i++) {
                const int64_t rgbaOffset = getRgbaOffsetForVoxelIndex(i, j, k);
                CaretAssertArrayIndex(mapRGBA, m_mapRGBACount, rgbaOffset);
                rgbaOut[rgbaOutIndex]   = mapRGBA[rgbaOffset];
                rgbaOut[rgbaOutIndex+1] = mapRGBA[rgbaOffset+1];
                rgbaOut[rgbaOutIndex+2] = mapRGBA[rgbaOffset+2];
                uint8_t alpha = mapRGBA[rgbaOffset+3];
                
                if (alpha > 0) {
                    if (labelTable != NULL) {
                        /*
                         * For label data, verify that the label is displayed.
                         * If NOT displayed, zero out the alpha value to
                         * prevent display of the data.
                         */
                        const int32_t dataValue = static_cast<int32_t>(m_volumeFile->getValue(i,
                                                                                              j,
                                                                                              k,
                                                                                              mapIndex));
                        const GiftiLabel* label = labelTable->getLabel(dataValue);
                        if (label != NULL) {
                            const GroupAndNameHierarchyItem* item = label->getGroupNameSelectionItem();
                            if (item != NULL) {
                                if (item->isSelected(displayGroup, tabIndex) == false) {
                                    alpha = 0;
                                }
                            }
                        }
                    }
                }
                
                if (alpha > 0.0) {
                    ++validVoxelCount;
                }
                rgbaOut[rgbaOutIndex+3] = alpha;
                rgbaOutIndex += 4;
            }
        }
    }
    
    return validVoxelCount;
}

/**
 * Get voxel coloring for a sub-slice in a map.  If voxel coloring is not ready
 * (it may be running in a different thread) this method will wait until the
 * coloring is valid prior to returning the slice's coloring.
 *
 * @param mapIndex
 *     Index of map.
 * @param slicePlane
 *    Plane of the slice.
 * @param sliceIndex
 *    Index of the slice.
 * @param firstCornerVoxelIndex 
 *    Indices of voxel for first corner of sub-slice (inclusive).
 * @param lastCornerVoxelIndex
 *    Indices of voxel for last corner of sub-slice (inclusive).
 * @param voxelCountIJK
 *    Voxel counts for each axis.
 * @param displayGroup
 *    The selected display group.
 * @param tabIndex
 *    Index of selected tab.
 * @param rgbaOut
 *    RGBA color components out.
 * @return
 *    Number of voxels with alpha greater than zero
 */
int64_t
VolumeFileVoxelColorizer::getVoxelColorsForSubSliceInMap(const int32_t mapIndex,
                                                         const VolumeSliceViewPlaneEnum::Enum slicePlane,
                                                         const int64_t sliceIndex,
                                                         const int64_t firstCornerVoxelIndex[3],
                                                         const int64_t lastCornerVoxelIndex[3],
                                                         const int64_t voxelCountIJK[3],
                                                         const DisplayGroupEnum::Enum displayGroup,
                                                         const int32_t tabIndex,
                                                         uint8_t* rgbaOut) const
{
    CaretAssertVectorIndex(m_mapRGBA, mapIndex);
    CaretAssert(sliceIndex >= 0);
    CaretAssert(rgbaOut);
    
    VolumeSpace::OrientTypes orient[3];
    m_volumeFile->getOrientation(orient);
    int orient2dim[3];
    int64_t incrementijk[3];
    
    for (int i = 0; i < 3; ++i)
    {
        incrementijk[i] = (lastCornerVoxelIndex[i] > firstCornerVoxelIndex[i]) ? 1 : -1;
        switch (orient[i])//easier to read than indexing by (orient[i] & 3)
        {
            case VolumeSpace::LEFT_TO_RIGHT:
            case VolumeSpace::RIGHT_TO_LEFT:
                orient2dim[0] = i;
                break;
            case VolumeSpace::POSTERIOR_TO_ANTERIOR:
            case VolumeSpace::ANTERIOR_TO_POSTERIOR:
                orient2dim[1] = i;
                break;
            case VolumeSpace::INFERIOR_TO_SUPERIOR:
            case VolumeSpace::SUPERIOR_TO_INFERIOR:
                orient2dim[2] = i;
                break;
        }
    }
    
    int outerLoop = -1, innerLoop = -1;
    int64_t iterijk[3];
    
    switch (slicePlane)
    {
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            outerLoop = orient2dim[2];
            innerLoop = orient2dim[1];
            iterijk[orient2dim[0]] = sliceIndex;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            outerLoop = orient2dim[2];
            innerLoop = orient2dim[0];
            iterijk[orient2dim[1]] = sliceIndex;
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            outerLoop = orient2dim[1];
            innerLoop = orient2dim[0];
            iterijk[orient2dim[2]] = sliceIndex;
            break;
        default:
            CaretAssert(false);
    }
    
    const int64_t voxelCount = (voxelCountIJK[0] * voxelCountIJK[1] * voxelCountIJK[2]);
    const int64_t rgbaCount = voxelCount * 4;
    
    /*
     * Pointer to maps RGBA values
     */
    const uint8_t* mapRGBA = m_mapRGBA[mapIndex];
    
    const GiftiLabelTable* labelTable = (m_volumeFile->isMappedWithLabelTable()
                                         ? m_volumeFile->getMapLabelTable(mapIndex)
                                         : NULL);
    if (m_volumeFile->isMappedWithLabelTable()) {
        CaretAssert(labelTable);
    }
    
    int64_t validVoxelCount = 0;
    
    int64_t innerCount = std::abs(lastCornerVoxelIndex[innerLoop] - firstCornerVoxelIndex[innerLoop]) + 1;//to check validity of index
    
    int64_t rgbaOutIndex = 0;
    for (iterijk[outerLoop] = firstCornerVoxelIndex[outerLoop];
         iterijk[outerLoop] != lastCornerVoxelIndex[outerLoop] + incrementijk[outerLoop];
         iterijk[outerLoop] += incrementijk[outerLoop])
    {
        for (iterijk[innerLoop] = firstCornerVoxelIndex[innerLoop];
            iterijk[innerLoop] != lastCornerVoxelIndex[innerLoop] + incrementijk[innerLoop];
            iterijk[innerLoop] += incrementijk[innerLoop])
        {
            CaretAssert(rgbaOutIndex == 4 * (innerCount * std::abs(iterijk[outerLoop] - firstCornerVoxelIndex[outerLoop]) +
                        std::abs(iterijk[innerLoop] - firstCornerVoxelIndex[innerLoop])));
            CaretAssertArrayIndex(rgbaOut, rgbaCount, rgbaOutIndex + 3);
            
            const int64_t rgbaOffset = getRgbaOffsetForVoxelIndex(iterijk[0], iterijk[1], iterijk[2]);
            CaretAssertArrayIndex(mapRGBA, m_mapRGBACount, rgbaOffset);
            
            rgbaOut[rgbaOutIndex]   = mapRGBA[rgbaOffset];
            rgbaOut[rgbaOutIndex+1] = mapRGBA[rgbaOffset+1];
            rgbaOut[rgbaOutIndex+2] = mapRGBA[rgbaOffset+2];
            uint8_t alpha = mapRGBA[rgbaOffset+3];
            
            if (alpha > 0)
            {
                if (labelTable != NULL)
                {
                    //For label data, verify that the label is displayed.
                    //If NOT displayed, zero out the alpha value to
                    //prevent display of the data.
                    
                    const int32_t dataValue = static_cast<int32_t>(m_volumeFile->getValue(iterijk, mapIndex));
                    const GiftiLabel* label = labelTable->getLabel(dataValue);
                    if (label != NULL)
                    {
                        const GroupAndNameHierarchyItem* item = label->getGroupNameSelectionItem();
                        if (item != NULL)
                        {
                            if (item->isSelected(displayGroup, tabIndex) == false)
                            {
                                alpha = 0;
                            }
                        }
                    }
                }
            }
            
            if (alpha > 0.0) {
                ++validVoxelCount;
            }
            rgbaOut[rgbaOutIndex+3] = alpha;

            rgbaOutIndex += 4;
        }
    }

    return validVoxelCount;
}

/**
 * Get the RGBA color components for voxel.
 *
 * @param i
 *    Parasaggital index
 * @param j
 *    Coronal index
 * @param k
 *    Axial index
 * @param mapIndex
 *    Index of map.
 * @param displayGroup
 *    The selected display group.
 * @param tabIndex
 *    Index of selected tab.
 * @param rgbaOut
 *    Contains voxel coloring on exit.
 */
void
VolumeFileVoxelColorizer::getVoxelColorInMap(const int64_t i,
                                             const int64_t j,
                                             const int64_t k,
                                             const int64_t mapIndex,
                                             const DisplayGroupEnum::Enum displayGroup,
                                             const int32_t tabIndex,
                                             uint8_t rgbaOut[4]) const
{
    /*
     * Pointer to maps RGBA values
     */
    CaretAssertVectorIndex(m_mapRGBA, mapIndex);
    const uint8_t* mapRGBA = m_mapRGBA[mapIndex];
    const int64_t rgbaOffset = getRgbaOffsetForVoxelIndex(i, j, k);
    CaretAssertArrayIndex(mapRGBA, m_mapRGBACount, rgbaOffset);
    rgbaOut[0] = mapRGBA[rgbaOffset];
    rgbaOut[1] = mapRGBA[rgbaOffset+1];
    rgbaOut[2] = mapRGBA[rgbaOffset+2];
    uint8_t alpha = mapRGBA[rgbaOffset+3];
    
    if (alpha > 0) {
        if (m_volumeFile->isMappedWithLabelTable()) {
            const GiftiLabelTable* labelTable = m_volumeFile->getMapLabelTable(mapIndex);
            CaretAssert(labelTable);
            /*
             * For label data, verify that the label is displayed.
             * If NOT displayed, zero out the alpha value to
             * prevent display of the data.
             */
            const int32_t dataValue = static_cast<int32_t>(m_volumeFile->getValue(i,
                                                                                  j,
                                                                                  k,
                                                                                  mapIndex));
            const GiftiLabel* label = labelTable->getLabel(dataValue);
            if (label != NULL) {
                const GroupAndNameHierarchyItem* item = label->getGroupNameSelectionItem();
                if (item != NULL) {
                    if (item->isSelected(displayGroup, tabIndex) == false) {
                        alpha = 0;
                    }
                }
            }
        }
    }
    
    rgbaOut[3] = alpha;
}

/**
 * Clear the voxel coloring for the given map.
 * @param mapIndex
 *    Index of map.
 */
void
VolumeFileVoxelColorizer::clearVoxelColoringForMap(const int64_t mapIndex)
{
    CaretAssertVectorIndex(m_mapRGBA, mapIndex);
    uint8_t* mapRGBA = m_mapRGBA[mapIndex];
    
    for (int64_t i = 0; i < m_mapRGBACount; i++) {
        mapRGBA[i] = 0.0;
    }
    
    CaretAssertVectorIndex(m_mapColoringValid, mapIndex);
    m_mapColoringValid[mapIndex] = false;
}

