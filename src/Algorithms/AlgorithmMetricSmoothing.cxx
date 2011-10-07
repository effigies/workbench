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

#include "AlgorithmMetricSmoothing.h"
#include "AlgorithmException.h"
#include "SurfaceFile.h"
#include "MetricFile.h"
#include "GeodesicHelper.h"
#include "TopologyHelper.h"
#include <cmath>

using namespace caret;
using namespace std;

AString AlgorithmMetricSmoothing::getCommandSwitch()
{
    return "-metric-smoothing";
}

AString AlgorithmMetricSmoothing::getShortDescription()
{
    return "SMOOTH A METRIC FILE";
}

AlgorithmParameters* AlgorithmMetricSmoothing::getParameters()
{
    AlgorithmParameters* ret = new AlgorithmParameters();
    ret->addSurfaceParameter(1, "surface", "the surface to smooth on");
    ret->addMetricParameter(2, "metric-in", "the metric to smooth");
    ret->addDoubleParameter(3, "smoothing-kernel", "the sigma for the gaussian kernel function, in mm");
    ret->addMetricOutputParameter(4, "metric-out", "the output metric");
    OptionalParameter* columnSelect = new OptionalParameter(5, "-column", "column select", "select a single column to smooth");
    columnSelect->addIntParameter(6, "column-number", "the column number to smooth");
    ret->addOptionalParameter(columnSelect);
    ret->setHelpText(
        AString("Smooth a metric file on a surface.  By default, smooths all input columns, specify -column to smooth ") +
        "only one column."
    );
    return ret;
}

void AlgorithmMetricSmoothing::useParameters(AlgorithmParameters* myParams, ProgressObject* myProgObj)
{
    SurfaceFile* mySurf = myParams->getSurface(1);
    MetricFile* myMetric = myParams->getMetric(2);
    double myKernel = myParams->getDouble(3);
    MetricFile* myMetricOut = myParams->getOutputMetric(4);
    OptionalParameter* columnSelect = myParams->getOptionalParameter(5);
    bool singleColumn = columnSelect->m_present;
    int64_t columnNum = -1;
    if (singleColumn)
    {
        columnNum = columnSelect->getInt(6);//todo: add one for 1-based conventions?
    }
    AlgorithmMetricSmoothing(myProgObj, mySurf, myMetric, myMetricOut, myKernel, columnNum);
}

AlgorithmMetricSmoothing::AlgorithmMetricSmoothing(ProgressObject* myProgObj, SurfaceFile* mySurf, MetricFile* myMetric, MetricFile* myMetricOut, double myKernel, int64_t columnNum) : AbstractAlgorithm(myProgObj)
{
    LevelProgress myProgress;
    if (myProgObj != NULL)
    {
        myProgress = myProgObj->startLevel(1.1f, getAlgorithmInternalWeight());
    }
    int32_t numNodes = mySurf->getNumberOfNodes();
    if (numNodes != myMetric->getNumberOfNodes())
    {
        throw AlgorithmException("metric does not match surface in number of nodes");
    }
    int32_t numCols = myMetric->getNumberOfColumns();
    if (columnNum < -1 || columnNum >= numCols)
    {
        throw AlgorithmException("invalid column number");
    }
    myProgress.setTask("Precomputing Smoothing Weights");
    precomputeWeights(mySurf, myKernel);
    myProgress.reportProgress(0.1f);
    if (columnNum == -1)
    {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, myMetric->getNumberOfColumns());
        for (int32_t col = 0; col < numCols; ++col)
        {
            myProgress.setTask("Smoothing Column " + AString::number(col));
            for (int32_t i = 0; i < numNodes; ++i)
            {
                float sum = 0.0f;
                int32_t numWeights = m_weightLists[i].m_nodes.size();
                for (int32_t j = 0; j < numWeights; ++j)
                {
                    sum += m_weightLists[i].m_weights[j] * myMetric->getValue(m_weightLists[i].m_nodes[j], col);
                }
                myMetricOut->setValue(i, col, sum / m_weightLists[i].m_weightSum);
            }
            myProgress.reportProgress(0.1f + ((float)col) / numCols);
        }
    } else {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        myProgress.setTask("Smoothing Column " + AString::number(columnNum));
        for (int32_t i = 0; i < numNodes; ++i)
        {
            float sum = 0.0f;
            int32_t numWeights = m_weightLists[i].m_nodes.size();
            for (int32_t j = 0; j < numWeights; ++j)
            {
                sum += m_weightLists[i].m_weights[j] * myMetric->getValue(m_weightLists[i].m_nodes[j], columnNum);
            }
            myMetricOut->setValue(i, 0, sum / m_weightLists[i].m_weightSum);
        }//should go incredibly fast, don't worry about progress for one column
    }
}

void AlgorithmMetricSmoothing::precomputeWeights(SurfaceFile* mySurf, double myKernel)
{
    GeodesicHelperBase myGeoBase(mySurf);
    int32_t numNodes = mySurf->getNumberOfNodes();
    float myKernelF = myKernel;
    float myGeoDist = myKernelF * 4.0f;
    float gaussianDenom = -0.5f / myKernelF / myKernelF;
    m_weightLists.resize(numNodes);
    TopologyHelper myTopoHelp(mySurf, false, true, false);
    GeodesicHelper myGeoHelp(myGeoBase);
    vector<float> distances;
    for (int32_t i = 0; i < numNodes; ++i)
    {
        myGeoHelp.getNodesToGeoDist(i, myGeoDist, m_weightLists[i].m_nodes, distances, true);
        if (distances.size() < 6)
        {
            myTopoHelp.getNodeNeighbors(i, m_weightLists[i].m_nodes);
            myGeoHelp.getGeoToTheseNodes(i, m_weightLists[i].m_nodes, distances, true);
        }
        int32_t numNeigh = (int32_t)distances.size();
        m_weightLists[i].m_weights.resize(numNeigh);
        m_weightLists[i].m_weightSum = 0.0f;
        for (int32_t j = 0; j < numNeigh; ++j)
        {
            float weight = exp(distances[j] * distances[j] * gaussianDenom);//exp(- dist ^ 2 / (2 * sigma ^ 2))
            m_weightLists[i].m_weights[j] = weight;
            m_weightLists[i].m_weightSum += weight;
        }
    }
}

float AlgorithmMetricSmoothing::getAlgorithmInternalWeight()
{
    return 1.0f;
}

float caret::AlgorithmMetricSmoothing::getSubAlgorithmWeight()
{
    return 0.0f;
}
