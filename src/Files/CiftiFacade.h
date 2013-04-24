#ifndef __CIFTI_FACADE_H__
#define __CIFTI_FACADE_H__

/*LICENSE_START*/
/*
 * Copyright 2013 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/


#include "CaretObject.h"
#include "CiftiXML.h"

namespace caret {

    class CiftiInterface;
    class GiftiMetaData;
    
    class CiftiFacade : public CaretObject {
        
    public:
        CiftiFacade(CiftiInterface* ciftiInterface);
        
        virtual ~CiftiFacade();
        
        bool getSurfaceMapForMappingDataToBrainordinates(std::vector<CiftiSurfaceMap>& mappingOut,
                                                         const StructureEnum::Enum structure) const;
        
        bool getVolumeMapForMappingDataToBrainordinates(std::vector<CiftiVolumeMap>& mappingOut) const;
        
        bool getMetadataForMapOrSeriesIndex(const int32_t mapIndex,
                                            GiftiMetaData* metadataOut);
        
        void setMetadataForMapOrSeriesIndex(const int32_t mapIndex,
                                            GiftiMetaData* metadataIn);
        
        GiftiLabelTable* getLabelTableForMapOrSeriesIndex(const int32_t mapIndex);
        
        PaletteColorMapping* getPaletteColorMappingForMapOrSeriesIndex(const int32_t mapIndex);
        
        AString getNameForMapOrSeriesIndex(const int32_t mapIndex) const;
        
        void setNameForMapOrSeriesIndex(const int32_t mapIndex,
                                        const AString name);
                                        
        bool getDataForMapOrSeriesIndex(const int32_t mapIndex,
                                        std::vector<float>& dataOut) const;
                                        
        // ADD_NEW_METHODS_HERE
        
    private:
        CiftiFacade(const CiftiFacade&);

        CiftiFacade& operator=(const CiftiFacade&);
        

        CiftiInterface* m_ciftiInterface;
    };
    
#ifdef __CIFTI_FACADE_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __CIFTI_FACADE_DECLARE__

} // namespace
#endif  //__CIFTI_FACADE_H__
