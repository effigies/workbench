/*LICENSE_START*/ 
/* 
 *  Copyright 1995-2002 Washington University School of Medicine 
 * 
 *  http://brainmap.wustl.edu 
 * 
 *  This file is part of CARET. 
 * 
 *  CARET is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version. 
 * 
 *  CARET is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 * 
 *  You should have received a copy of the GNU General Public License 
 *  along with CARET; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 */ 

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "ClassAndNameHierarchyModel.h"
#include "DataFileTypeEnum.h"
#include "GiftiFile.h"
#include "MathFunctions.h"
#include "LabelFile.h"

using namespace caret;

/**
 * Constructor.
 */
LabelFile::LabelFile()
: GiftiTypeFile(DataFileTypeEnum::LABEL)
{
    m_classNameHierarchy = NULL;
    this->initializeMembersLabelFile();
}

/**
 * Copy constructor.
 *
 * @param sf
 *     Surface file that is copied.
 */
LabelFile::LabelFile(const LabelFile& sf)
: GiftiTypeFile(sf)
{
    m_classNameHierarchy = NULL;
    this->copyHelperLabelFile(sf);
}


/**
 * Assignment operator.
 *
 * @param sf
 *     Surface file that is copied.
 * @return
 *     This surface file with content replaced
 *     by the LabelFile parameter.
 */
LabelFile& 
LabelFile::operator=(const LabelFile& sf)
{
    if (this != &sf) {
        GiftiTypeFile::operator=(sf);
        this->copyHelperLabelFile(sf);
    }
    return *this;
}

/**
 * Destructor.
 */
LabelFile::~LabelFile()
{
    this->columnDataPointers.clear();
    delete m_classNameHierarchy;
}

/**
 * Clear the surface file.
 */
void 
LabelFile::clear()
{
    GiftiTypeFile::clear();
    this->columnDataPointers.clear();
    m_classNameHierarchy->clear();
}

/** 
 * @return Return the GIFTI Label Table. 
 */
GiftiLabelTable* 
LabelFile::getLabelTable() 
{ 
    return this->giftiFile->getLabelTable(); 
}

/** 
 * @return Return the GIFTI Label Table. 
 */
const GiftiLabelTable* 
LabelFile::getLabelTable() const 
{ 
    return this->giftiFile->getLabelTable(); 
}

/**
 * @return The class and name hierarchy.
 */
ClassAndNameHierarchyModel*
LabelFile::getClassAndNameHierarchyModel()
{
    m_classNameHierarchy->update(this,
                                 m_forceUpdateOfClassAndNameHierarchy);
    m_forceUpdateOfClassAndNameHierarchy = false;
    
    return m_classNameHierarchy;
}

/**
 * Validate the contents of the file after it
 * has been read such as correct number of 
 * data arrays and proper data types/dimensions.
 */
void 
LabelFile::validateDataArraysAfterReading() throw (DataFileException)
{
    this->columnDataPointers.clear();

    this->initializeMembersLabelFile();
    
    this->verifyDataArraysHaveSameNumberOfRows(0, 0);
    
    const int32_t numberOfDataArrays = this->giftiFile->getNumberOfDataArrays();
    for (int32_t i = 0; i < numberOfDataArrays; i++) {
        this->columnDataPointers.push_back(this->giftiFile->getDataArray(i)->getDataPointerInt());
    }
    
    m_classNameHierarchy->update(this,
                                 true);
    m_forceUpdateOfClassAndNameHierarchy = false;
    m_classNameHierarchy->setAllSelected(true);
    
    CaretLogFiner("CLASS/NAME Table for : "
                  + this->getFileNameNoPath()
                  + "\n"
                  + m_classNameHierarchy->toString());
}

/**
 * Get the number of nodes.
 *
 * @return
 *    The number of nodes.
 */
int32_t
LabelFile::getNumberOfNodes() const
{
    int32_t numNodes = 0;
    int32_t numDataArrays = this->giftiFile->getNumberOfDataArrays();
    if (numDataArrays > 0) {
        numNodes = this->giftiFile->getDataArray(0)->getNumberOfRows();
    }
    return numNodes;
}

/**
 * Get the number of columns.
 *
 * @return
 *   The number of columns.
 */
int32_t
LabelFile::getNumberOfColumns() const
{
    const int32_t numCols = this->giftiFile->getNumberOfDataArrays();
    return numCols;
}


/**
 * Initialize members of this class.
 */
void 
LabelFile::initializeMembersLabelFile()
{
    if (m_classNameHierarchy != NULL) {
        delete m_classNameHierarchy;
    }
    m_classNameHierarchy = new ClassAndNameHierarchyModel();
    m_forceUpdateOfClassAndNameHierarchy = true;
}

/**
 * Helps copying files.
 *
 * @param sf
 *    File that is copied.
 */
void 
LabelFile::copyHelperLabelFile(const LabelFile& /*sf*/)
{
    if (m_classNameHierarchy != NULL) {
        delete m_classNameHierarchy;
    }
    m_classNameHierarchy = new ClassAndNameHierarchyModel();
    m_forceUpdateOfClassAndNameHierarchy = true;
    
    this->validateDataArraysAfterReading();
}

/**
 * Get label name for a node.
 * 
 * @param nodeIndex
 *     Node index.
 * @param columnIndex
 *     Column index.
 * @return
 *     Label name at the given node and column indices
 *     Empty string if label is not available.
 */
AString 
LabelFile::getLabelName(const int32_t nodeIndex,
                        const int32_t columnIndex) const
{
    const int32_t labelKey = this->getLabelKey(nodeIndex, columnIndex);
    AString label = this->giftiFile->getLabelTable()->getLabelName(labelKey);
    return label;
}

/**
 * Get label key for a node.
 * 
 * @param nodeIndex
 *     Node index.
 * @param columnIndex
 *     Column index.
 * @return
 *     Label key at the given node and column indices.
 */
int32_t 
LabelFile::getLabelKey(const int32_t nodeIndex,
                       const int32_t columnIndex) const
{
    CaretAssertVectorIndex(this->columnDataPointers, columnIndex);
    CaretAssertMessage((nodeIndex >= 0) && (nodeIndex < this->getNumberOfNodes()), 
                       "Node Index out of range.");
    
    return this->columnDataPointers[columnIndex][nodeIndex];
}

/**
 * set label key for a node.
 * 
 * @param nodeIndex
 *     Node index.
 * @param columnIndex
 *     Column index.
 * param labelKey
 *     Label key inserted at the given node and column indices.
 */
void 
LabelFile::setLabelKey(const int32_t nodeIndex,
                       const int32_t columnIndex,
                       const int32_t labelKey)
{
    CaretAssertVectorIndex(this->columnDataPointers, columnIndex);
    CaretAssertMessage((nodeIndex >= 0) && (nodeIndex < this->getNumberOfNodes()), "Node Index out of range.");
    
    this->columnDataPointers[columnIndex][nodeIndex] = labelKey;
    this->setModified();
    m_forceUpdateOfClassAndNameHierarchy = true;
}

/**
 * Get nodes in the given column with the given lable key.
 * @param columnIndex
 *    Index of column
 * @param labelKey
 *    Key of label that is desired.
 * @param nodeIndicesOut
 *    On exit, will contain indices of nodes that have the 
 *    given label key in the given column.
 */
void 
LabelFile::getNodeIndicesWithLabelKey(const int32_t columnIndex,
                                const int32_t labelKey,
                                std::vector<int32_t>& nodeIndicesOut) const
{
    const int32_t numberOfNodes = this->getNumberOfNodes();
    nodeIndicesOut.clear();
    nodeIndicesOut.reserve(numberOfNodes);
    
    for (int32_t i = 0; i < numberOfNodes; i++) {
        if (this->getLabelKey(i, columnIndex) == labelKey) {
            nodeIndicesOut.push_back(i);
        }
    }
}

/**
 * Get a pointer to the keys for a label file column.
 * @param columnIndex
 *     Index of the column.
 * @return 
 *     Pointer to keys for the given column.
 */
const int32_t* 
LabelFile::getLabelKeyPointerForColumn(const int32_t columnIndex) const
{
    CaretAssertVectorIndex(this->columnDataPointers, columnIndex);
    return this->columnDataPointers[columnIndex];    
}

void LabelFile::setNumberOfNodesAndColumns(int32_t nodes, int32_t columns)
{
    giftiFile->clear();
    columnDataPointers.clear();

    const int32_t unassignedKey = this->getLabelTable()->getUnassignedLabelKey();
    
    std::vector<int64_t> dimensions;
    dimensions.push_back(nodes);
    for (int32_t i = 0; i < columns; ++i)
    {
        giftiFile->addDataArray(new GiftiDataArray(NiftiIntentEnum::NIFTI_INTENT_LABEL, NiftiDataTypeEnum::NIFTI_TYPE_INT32, dimensions, GiftiEncodingEnum::GZIP_BASE64_BINARY));
        columnDataPointers.push_back(giftiFile->getDataArray(i)->getDataPointerInt());
        int32_t* ptr = giftiFile->getDataArray(i)->getDataPointerInt();
        for (int32_t j = 0; j < nodes; j++) {
            ptr[j] = unassignedKey;
        }
    }
    setModified();
    m_forceUpdateOfClassAndNameHierarchy = true;
}

/**
 * Add map(s) to this GIFTI file.
 * @param numberOfNodes
 *     Number of nodes.  If file is not empty, this value must
 *     match the number of nodes that are in the file.
 * @param numberOfMaps
 *     Number of maps to add.
 */
void 
LabelFile::addMaps(const int32_t numberOfNodes,
                       const int32_t numberOfMaps) throw (DataFileException)
{
    if (numberOfNodes <= 0) {
        throw DataFileException("When adding maps to "
                                + this->getFileNameNoPath()
                                + " the number of nodes must be greater than zero");
    }
    
    if (this->getNumberOfNodes() > 0) {
        if (numberOfNodes != this->getNumberOfNodes()) {
            throw DataFileException("When adding maps to "
                                    + this->getFileNameNoPath()
                                    + " the requested number of nodes is "
                                    + AString::number(numberOfNodes)
                                    + " but the file contains "
                                    + AString::number(this->getNumberOfNodes())
                                    + " nodes.");
        }
    }
    
    if (numberOfMaps <= 0) {
        throw DataFileException("When adding maps, the number of maps must be greater than zero.");
    }
    
    const int32_t unassignedKey = this->getLabelTable()->getUnassignedLabelKey();
    
    if ((this->getNumberOfNodes() > 0) 
        && (this->getNumberOfMaps() > 0)) {
        std::vector<int64_t> dimensions;
        dimensions.push_back(numberOfNodes);
        
        for (int32_t i = 0; i < numberOfMaps; ++i)
        {
            this->giftiFile->addDataArray(new GiftiDataArray(NiftiIntentEnum::NIFTI_INTENT_LABEL, 
                                                             NiftiDataTypeEnum::NIFTI_TYPE_INT32, 
                                                             dimensions, 
                                                             GiftiEncodingEnum::GZIP_BASE64_BINARY));
            const int32_t mapIndex = giftiFile->getNumberOfDataArrays() - 1;
            this->columnDataPointers.push_back(giftiFile->getDataArray(mapIndex)->getDataPointerInt());
            
            int32_t* ptr = giftiFile->getDataArray(mapIndex)->getDataPointerInt();
            for (int32_t j = 0; j < numberOfNodes; j++) {
                ptr[j] = unassignedKey;
            }
        }
    }
    else {
        this->setNumberOfNodesAndColumns(numberOfNodes, 
                                         numberOfMaps);
    }
    
    m_forceUpdateOfClassAndNameHierarchy = true;
    this->setModified();
}

void LabelFile::setLabelKeysForColumn(const int32_t columnIndex, const int32_t* valuesIn)
{
    CaretAssertVectorIndex(this->columnDataPointers, columnIndex);
    int32_t* myColumn = columnDataPointers[columnIndex];
    int numNodes = (int)getNumberOfNodes();
    for (int i = 0; i < numNodes; ++i)
    {
        myColumn[i] = valuesIn[i];
    }
    m_forceUpdateOfClassAndNameHierarchy = true;
    setModified();
}

/**
 * Return a vector containing the keys used in a map.  Each key is listed
 * once and the keys will be in ascending order. 
 * @param mapIndex
 *   Index of map.
 * @return
 *   Vector containing the keys.
 */
std::vector<int32_t>
LabelFile::getUniqueLabelKeysUsedInMap(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(this->columnDataPointers, mapIndex);
    
    std::set<int32_t> uniqueKeys;
    const int32_t numNodes = getNumberOfNodes();
    for (int32_t i = 0; i < numNodes; i++) {
        const int32_t key = getLabelKey(i, mapIndex);
        uniqueKeys.insert(key);
    }
    
    std::vector<int32_t> keyVector;
    keyVector.insert(keyVector.end(),
                     uniqueKeys.begin(),
                     uniqueKeys.end());
    return keyVector;
}


