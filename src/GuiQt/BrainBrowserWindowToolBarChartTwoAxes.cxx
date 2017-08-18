
/*LICENSE_START*/
/*
 *  Copyright (C) 2014 Washington University School of Medicine
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

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

#define __BRAIN_BROWSER_WINDOW_TOOL_BAR_CHART_TWO_AXES_DECLARE__
#include "BrainBrowserWindowToolBarChartTwoAxes.h"
#undef __BRAIN_BROWSER_WINDOW_TOOL_BAR_CHART_TWO_AXES_DECLARE__

#include "AnnotationPercentSizeText.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "CaretDataFile.h"
#include "CaretDataFileSelectionModel.h"
#include "CaretMappableDataFile.h"
#include "ChartTwoCartesianAxis.h"
#include "ChartTwoOverlay.h"
#include "ChartTwoOverlaySet.h"
#include "ChartableTwoFileBaseChart.h"
#include "ChartableTwoFileDelegate.h"
#include "ChartableTwoFileHistogramChart.h"
#include "ChartableTwoFileLineSeriesChart.h"
#include "EnumComboBoxTemplate.h"
#include "EventBrowserWindowGraphicsRedrawn.h"
#include "EventChartTwoAttributesChanged.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "ModelChartTwo.h"
#include "WuQDataEntryDialog.h"
#include "WuQFactory.h"
#include "WuQWidgetObjectGroup.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::BrainBrowserWindowToolBarChartTwoAxes 
 * \brief Controls for chart attributes.
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * @param parentToolBar
 *   The parent toolbar.
 */
BrainBrowserWindowToolBarChartTwoAxes::BrainBrowserWindowToolBarChartTwoAxes(BrainBrowserWindowToolBar* parentToolBar)
: BrainBrowserWindowToolBarComponent(parentToolBar)
{
    m_chartOverlaySet = NULL;
    m_chartAxis = NULL;
    
    m_axisDisplayedByUserCheckBox = new QCheckBox("Show Axis");
    m_axisDisplayedByUserCheckBox->setToolTip("Show/hide the axis");
    QObject::connect(m_axisDisplayedByUserCheckBox, &QCheckBox::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChanged);

    /*
     * Axes selection
     */
    m_axisComboBox = new EnumComboBoxTemplate(this);
    m_axisComboBox->setup<ChartAxisLocationEnum, ChartAxisLocationEnum::Enum>();
    QObject::connect(m_axisComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::axisChanged);
    m_axisComboBox->getWidget()->setToolTip("Choose axis for editing");
    
    /*
     * Controls for axis parameters
     */
    m_axisLabelToolButton = new QToolButton();
    m_axisLabelToolButton->setText("Edit Label...");
    QObject::connect(m_axisLabelToolButton, &QToolButton::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::axisLabelToolButtonClicked);
    WuQtUtilities::setToolButtonStyleForQt5Mac(m_axisLabelToolButton);
    m_axisLabelToolButton->setToolTip("Edit the axis name for the file in the selected overlay");
    
    m_autoUserRangeComboBox = new EnumComboBoxTemplate(this);
    m_autoUserRangeComboBox->setup<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>();
    QObject::connect(m_autoUserRangeComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChanged);
    m_autoUserRangeComboBox->getWidget()->setToolTip("Choose auto or user axis range scaling");
    
    const double bigValue = 999999.0;
    m_userMinimumValueSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(-bigValue, bigValue, 1.0, 1);
    QObject::connect(m_userMinimumValueSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::axisMinimumValueChanged);
    m_userMinimumValueSpinBox->setToolTip("Set user scaling axis minimum value");
    
    m_userMaximumValueSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(-bigValue, bigValue, 1.0, 1);
    QObject::connect(m_userMaximumValueSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::axisMaximumValueChanged);
    m_userMaximumValueSpinBox->setToolTip("Set user scaling axis maximum value");
    
    m_showTickMarksCheckBox = new QCheckBox("Show Ticks");
    QObject::connect(m_showTickMarksCheckBox, &QCheckBox::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedBool);
    m_showTickMarksCheckBox->setToolTip("Show ticks along the axis");
    
    m_showLabelCheckBox = new QCheckBox("Show Label");
    QObject::connect(m_showLabelCheckBox, &QCheckBox::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedBool);
    m_showLabelCheckBox->setToolTip("Show label on axis");
    
    QLabel* axisLabelFromOverlayLabel = new QLabel("Label From File In");
    m_axisLabelFromOverlayComboBox = new QComboBox();
    m_axisLabelFromOverlayComboBox->setToolTip("Label for axis is from file in selected layer");
    QObject::connect(m_axisLabelFromOverlayComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedInt);
    
    m_userNumericFormatComboBox = new EnumComboBoxTemplate(this);
    m_userNumericFormatComboBox->setup<NumericFormatModeEnum, NumericFormatModeEnum::Enum>();
    QObject::connect(m_userNumericFormatComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChanged);
    m_userNumericFormatComboBox->getWidget()->setToolTip("Choose format of axis scale numeric values");
    
    m_userDigitsRightOfDecimalSpinBox = WuQFactory::newSpinBoxWithMinMaxStep(0, 10, 1);
    QObject::connect(m_userDigitsRightOfDecimalSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedInt);
    m_userDigitsRightOfDecimalSpinBox->setToolTip("Set digits right of decimal for\ndecimal or scientific format");
    
    m_numericSubdivisionsModeComboBox = new EnumComboBoxTemplate(this);
    m_numericSubdivisionsModeComboBox->setup<ChartTwoNumericSubdivisionsModeEnum, ChartTwoNumericSubdivisionsModeEnum::Enum>();
    QObject::connect(m_numericSubdivisionsModeComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChanged);
    m_numericSubdivisionsModeComboBox->getWidget()->setToolTip("Numeric subdivisions mode");
    
    m_userSubdivisionsSpinBox = WuQFactory::newSpinBoxWithMinMaxStep(0, 100, 1);
    QObject::connect(m_userSubdivisionsSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedInt); 
    m_userSubdivisionsSpinBox->setToolTip("Set subdivisions on the axis when Auto is not checked");
    
    /*
     * Size spin boxes
     */
    m_labelSizeSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(0.0, 100.0, 1.0, 1);
    m_labelSizeSpinBox->setSuffix("%");
    QObject::connect(m_labelSizeSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedDouble);
    m_labelSizeSpinBox->setToolTip("Set height of label as percentage of tab height");
    
    m_numericsSizeSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(0.0, 100.0, 1.0, 1);
    m_numericsSizeSpinBox->setSuffix("%");
    QObject::connect(m_numericsSizeSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedDouble);
    m_numericsSizeSpinBox->setToolTip("Set height of numeric values as percentage of tab height");
    
    m_linesTicksSizeSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(0.0, 100.0, 1.0, 1);
    m_linesTicksSizeSpinBox->setSuffix("%");
    QObject::connect(m_linesTicksSizeSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedDouble);
    m_linesTicksSizeSpinBox->setToolTip("Set thickness of axis lines and ticks as percentage of tab height");
    
    m_paddingSizeSpinBox = WuQFactory::newDoubleSpinBoxWithMinMaxStepDecimals(0.0, 100.0, 1.0, 1);
    m_paddingSizeSpinBox->setSuffix("%");
    QObject::connect(m_paddingSizeSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarChartTwoAxes::valueChangedDouble);
    m_paddingSizeSpinBox->setToolTip("Set padding (space between edge and labels) as percentage of tab height");
    
    /*
     * Group widgets for blocking signals
     */
    m_widgetGroup = new WuQWidgetObjectGroup(this);
    m_widgetGroup->add(m_axisComboBox);
    m_widgetGroup->add(m_axisDisplayedByUserCheckBox);
    m_widgetGroup->add(m_axisLabelToolButton);
    m_widgetGroup->add(m_autoUserRangeComboBox->getWidget());
    m_widgetGroup->add(m_userMinimumValueSpinBox);
    m_widgetGroup->add(m_userMaximumValueSpinBox);
    m_widgetGroup->add(m_showTickMarksCheckBox);
    m_widgetGroup->add(m_showLabelCheckBox);
    m_widgetGroup->add(m_axisLabelFromOverlayComboBox);
    m_widgetGroup->add(m_userNumericFormatComboBox->getWidget());
    m_widgetGroup->add(m_userDigitsRightOfDecimalSpinBox);
    m_widgetGroup->add(m_numericSubdivisionsModeComboBox->getWidget());
    m_widgetGroup->add(m_userSubdivisionsSpinBox);
    m_widgetGroup->add(m_labelSizeSpinBox);
    m_widgetGroup->add(m_numericsSizeSpinBox);
    m_widgetGroup->add(m_linesTicksSizeSpinBox);
    m_widgetGroup->add(m_paddingSizeSpinBox);
    
    /*
     * Size layout
     */
    QWidget* sizesWidget = new QWidget();
    QGridLayout* sizesLayout = new QGridLayout(sizesWidget);
    WuQtUtilities::setLayoutSpacingAndMargins(sizesLayout, 3, 0);
    int32_t sizesRow = 0;
    sizesLayout->addWidget(new QLabel("Label"), sizesRow, 0);
    sizesLayout->addWidget(m_labelSizeSpinBox, sizesRow, 1);
    sizesRow++;
    sizesLayout->addWidget(new QLabel("Numerics"), sizesRow, 0);
    sizesLayout->addWidget(m_numericsSizeSpinBox, sizesRow, 1);
    sizesRow++;
    sizesLayout->addWidget(new QLabel("Lines/Ticks"), sizesRow, 0);
    sizesLayout->addWidget(m_linesTicksSizeSpinBox, sizesRow, 1);
    sizesRow++;
    sizesLayout->addWidget(new QLabel("Padding"), sizesRow, 0);
    sizesLayout->addWidget(m_paddingSizeSpinBox, sizesRow, 1);
    sizesRow++;
    
    /*
     * Show widgets layout
     */
    QWidget* showWidget = new QWidget();
    QGridLayout* showLayout = new QGridLayout(showWidget);
    WuQtUtilities::setLayoutSpacingAndMargins(showLayout, 8, 0);
    int32_t axisRow = 0;
    showLayout->addWidget(m_axisDisplayedByUserCheckBox, axisRow, 0);
    axisRow++;
    showLayout->addWidget(m_showLabelCheckBox, axisRow, 0);
    axisRow++;
    showLayout->addWidget(m_showTickMarksCheckBox, axisRow, 0);
    axisRow++;
    showWidget->setSizePolicy(showWidget->sizePolicy().horizontalPolicy(),
                              QSizePolicy::Fixed);
    
    /*
     * Range widgets layout
     */
    QWidget* rangeWidget = new QWidget();
    QGridLayout* rangeLayout = new QGridLayout(rangeWidget);
    WuQtUtilities::setLayoutSpacingAndMargins(rangeLayout, 3, 0);
    int rangeRow = 0;
    rangeLayout->addWidget(new QLabel("Range Mode"), rangeRow, 0);
    m_autoUserRangeComboBox->getComboBox()->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    rangeLayout->addWidget(m_autoUserRangeComboBox->getWidget(), rangeRow, 1);
    rangeRow++;
    m_userMinimumValueSpinBox->setFixedWidth(90);
    m_userMaximumValueSpinBox->setFixedWidth(90);
    rangeLayout->addWidget(new QLabel("User Max"), rangeRow, 0);
    rangeLayout->addWidget(m_userMaximumValueSpinBox, rangeRow, 1);
    rangeRow++;
    rangeLayout->addWidget(new QLabel("User Min"), rangeRow, 0);
    rangeLayout->addWidget(m_userMinimumValueSpinBox, rangeRow, 1);
    
    /*
     * Numerics widgets layout
     */
    QWidget* numericsWidget = new QWidget();
    QGridLayout* numericsLayout = new QGridLayout(numericsWidget);
    WuQtUtilities::setLayoutSpacingAndMargins(numericsLayout, 3, 0);
    int COLUMN_ONE = 0;
    int COLUMN_TWO = 1;
    int COLUMN_THREE = 2;
    int numericsRow = 0;
    numericsLayout->addWidget(new QLabel("Format"), numericsRow, COLUMN_ONE);
    numericsLayout->addWidget(m_userNumericFormatComboBox->getWidget(), numericsRow, COLUMN_TWO, 1, 2);
    numericsRow++;
    numericsLayout->addWidget(new QLabel("Decimals"), numericsRow, COLUMN_ONE);
    numericsLayout->addWidget(m_userDigitsRightOfDecimalSpinBox, numericsRow, COLUMN_TWO, 1, 2);
    numericsRow++;
    numericsLayout->addWidget(new QLabel("Subdivisions"), numericsRow, COLUMN_ONE);
    numericsLayout->addWidget(m_numericSubdivisionsModeComboBox->getWidget(), numericsRow, COLUMN_TWO);
    numericsLayout->addWidget(m_userSubdivisionsSpinBox, numericsRow, COLUMN_THREE);
    
    /*
     * Top layout
     */
    QHBoxLayout* topLayout = new QHBoxLayout();
    WuQtUtilities::setLayoutSpacingAndMargins(topLayout, 3, 0);
    topLayout->addWidget(new QLabel("Edit Axis "));
    topLayout->addWidget(m_axisComboBox->getWidget());
    topLayout->addSpacing(10);
    topLayout->addStretch();
    topLayout->addWidget(axisLabelFromOverlayLabel);
    topLayout->addWidget(m_axisLabelFromOverlayComboBox);
    topLayout->addSpacing(10);
    topLayout->addWidget(m_axisLabelToolButton);
    
    /*
     * Grid layout containing layouts
     */
    QGridLayout* gridLayout = new QGridLayout();
    WuQtUtilities::setLayoutSpacingAndMargins(gridLayout, 3, 0);
    gridLayout->addLayout(topLayout, 0, 0, 1, 7);
    gridLayout->addWidget(showWidget, 1, 0, Qt::AlignTop);
    gridLayout->addWidget(WuQtUtilities::createVerticalLineWidget(), 1, 1);
    gridLayout->addWidget(sizesWidget, 1, 2, Qt::AlignTop);
    gridLayout->addWidget(WuQtUtilities::createVerticalLineWidget(), 1, 3);
    gridLayout->addWidget(rangeWidget, 1, 4, Qt::AlignTop);
    gridLayout->addWidget(WuQtUtilities::createVerticalLineWidget(), 1, 5);
    gridLayout->addWidget(numericsWidget, 1, 6, Qt::AlignTop);
    
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    WuQtUtilities::setLayoutSpacingAndMargins(dialogLayout, 0, 2);
    dialogLayout->addLayout(gridLayout);
    dialogLayout->addStretch();

    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN);
}

/**
 * Destructor.
 */
BrainBrowserWindowToolBarChartTwoAxes::~BrainBrowserWindowToolBarChartTwoAxes()
{
}

/**
 * Receive an event.
 *
 * @param event
 *    The event.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN) {
        EventBrowserWindowGraphicsRedrawn* redrawEvent = dynamic_cast<EventBrowserWindowGraphicsRedrawn*>(event);
        CaretAssert(redrawEvent);
        updateContent(getTabContentFromSelectedTab());
    }
    else {
        BrainBrowserWindowToolBarComponent::receiveEvent(event);
    }
}

/**
 * Update content of this tool bar component.
 *
 * @param browserTabContent
 *     Content of the browser tab.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::updateContent(BrowserTabContent* browserTabContent)
{
    if (browserTabContent == NULL) {
        setEnabled(false);
        return;
    }
    
    updateControls(browserTabContent);
}

/**
 * Get the selection data.
 *
 * @param browserTabContent
 *     The tab content.
 * @param chartOverlaySetOut
 *     The chart overlay set (may be NULL)
 * @param validAxesLocationsOut
 *     The valid axes locations.
 * @param selectedAxisOut
 *     Output with selected axis (may be NULL)
 * @param axisLabelOut
 *     Output with the axis text label.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::getSelectionData(BrowserTabContent* browserTabContent,
                                                        ChartTwoOverlaySet* &chartOverlaySetOut,
                                                        std::vector<ChartAxisLocationEnum::Enum>& validAxesLocationsOut,
                                                        ChartTwoCartesianAxis* &selectedAxisOut,
                                                        AnnotationPercentSizeText* &axisLabelOut) const
{
    chartOverlaySetOut = NULL;
    validAxesLocationsOut.clear();
    selectedAxisOut = NULL;
    axisLabelOut = NULL;
    
    if (browserTabContent == NULL) {
        return;
    }
    
    const ChartAxisLocationEnum::Enum lastSelectedAxis = getSelectedAxisLocation();
    
    if (browserTabContent != NULL) {
        ModelChartTwo* modelChartTwo = browserTabContent->getDisplayedChartTwoModel();
        const int32_t tabIndex = browserTabContent->getTabNumber();
        if (modelChartTwo != NULL) {
            switch (modelChartTwo->getSelectedChartTwoDataType(tabIndex)) {
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_INVALID:
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_HISTOGRAM:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_SERIES:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_MATRIX:
                    break;
            }
            
            if (chartOverlaySetOut != NULL) {
                int32_t defaultAxisIndex = -1;
                
                std::vector<ChartTwoCartesianAxis*> axes;
                AnnotationPercentSizeText* leftAxisLabel   = NULL;
                AnnotationPercentSizeText* rightAxisLabel  = NULL;
                AnnotationPercentSizeText* bottomAxisLabel = NULL;
                chartOverlaySetOut->getDisplayedChartAxes(axes,
                                                       leftAxisLabel,
                                                       rightAxisLabel,
                                                       bottomAxisLabel);
                const int32_t numAxes = static_cast<int32_t>(axes.size());
                for (int32_t i = 0; i < numAxes; i++) {
                    const ChartAxisLocationEnum::Enum axisLocation = axes[i]->getAxisLocation();
                    if (lastSelectedAxis == axisLocation) {
                        defaultAxisIndex = i;
                    }
                    validAxesLocationsOut.push_back(axisLocation);
                }
                CaretAssert(validAxesLocationsOut.size() == axes.size());
                
                if (defaultAxisIndex < 0) {
                    /*
                     * If selected axis not found, switch to opposite axis
                     * User may have switched vertical axis from left to right
                     */
                    ChartAxisLocationEnum::Enum oppositeAxis = ChartAxisLocationEnum::getOppositeAxis(lastSelectedAxis);
                    for (int32_t i = 0; i < numAxes; i++) {
                        if (oppositeAxis == axes[i]->getAxisLocation()) {
                            defaultAxisIndex = i;
                            break;
                        }
                    }
                }
                
                if ( ! axes.empty()) {
                    if (defaultAxisIndex < 0) {
                        defaultAxisIndex = 0;
                    }
                    CaretAssertVectorIndex(axes, defaultAxisIndex);
                    selectedAxisOut = axes[defaultAxisIndex];
                }
                
                if (selectedAxisOut != NULL) {
                    axisLabelOut = chartOverlaySetOut->getAxisLabel(selectedAxisOut);
                }
            }
        }
    }
    
}

/**
 * Called when axis is changed.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::axisChanged()
{
    updateContent(getTabContentFromSelectedTab());
}

/**
 * @return The selected axes location (will return left even if no valid selection).
 */
ChartAxisLocationEnum::Enum
BrainBrowserWindowToolBarChartTwoAxes::getSelectedAxisLocation() const
{
    ChartAxisLocationEnum::Enum axis = ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT;
    
    if (m_axisComboBox->getComboBox()->count() > 0) {
        axis = m_axisComboBox->getSelectedItem<ChartAxisLocationEnum, ChartAxisLocationEnum::Enum>();
    }
    
    return axis;
}


void
BrainBrowserWindowToolBarChartTwoAxes::updateControls(BrowserTabContent* browserTabContent)
{
    m_chartOverlaySet = NULL;
    std::vector<ChartAxisLocationEnum::Enum> validAxesLocations;
    ChartTwoCartesianAxis* selectedAxis = NULL;
    AnnotationPercentSizeText* axisLabel = NULL;
    
    getSelectionData(browserTabContent,
                     m_chartOverlaySet,
                     validAxesLocations,
                     selectedAxis,
                     axisLabel);
    
    m_widgetGroup->blockAllSignals(true);
    
    m_chartAxis = selectedAxis;
    if ((m_chartOverlaySet != NULL)
        && (m_chartAxis != NULL)) {
        m_axisComboBox->setupWithItems<ChartAxisLocationEnum, ChartAxisLocationEnum::Enum>(validAxesLocations);
        m_axisComboBox->setSelectedItem<ChartAxisLocationEnum, ChartAxisLocationEnum::Enum>(m_chartAxis->getAxisLocation());
        
        m_axisDisplayedByUserCheckBox->setChecked(m_chartAxis->isDisplayedByUser());
        m_autoUserRangeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(m_chartAxis->getScaleRangeMode());
        float rangeMin(0.0), rangeMax(0.0);
        m_chartAxis->getRange(rangeMin, rangeMax);
        m_userMinimumValueSpinBox->setRange(rangeMin, rangeMax);
        m_userMinimumValueSpinBox->setValue(m_chartAxis->getUserScaleMinimumValue());
        m_userMaximumValueSpinBox->setRange(rangeMin, rangeMax);
        m_userMaximumValueSpinBox->setValue(m_chartAxis->getUserScaleMaximumValue());
        m_showTickMarksCheckBox->setChecked(m_chartAxis->isShowTickmarks());
        m_showLabelCheckBox->setChecked(m_chartAxis->isShowLabel());
        const NumericFormatModeEnum::Enum numericFormat = m_chartAxis->getUserNumericFormat();
        m_userNumericFormatComboBox->setSelectedItem<NumericFormatModeEnum, NumericFormatModeEnum::Enum>(numericFormat);
        m_userDigitsRightOfDecimalSpinBox->setValue(m_chartAxis->getUserDigitsRightOfDecimal());
        m_userDigitsRightOfDecimalSpinBox->setEnabled(numericFormat != NumericFormatModeEnum::AUTO);
        m_numericSubdivisionsModeComboBox->setSelectedItem<ChartTwoNumericSubdivisionsModeEnum, ChartTwoNumericSubdivisionsModeEnum::Enum>(m_chartAxis->getNumericSubdivsionsMode());
        m_userSubdivisionsSpinBox->setValue(m_chartAxis->getUserNumberOfSubdivisions());
        m_userSubdivisionsSpinBox->setEnabled( m_chartAxis->getNumericSubdivsionsMode() == ChartTwoNumericSubdivisionsModeEnum::USER);
        
        m_labelSizeSpinBox->setValue(m_chartAxis->getLabelTextSize());
        m_numericsSizeSpinBox->setValue(m_chartAxis->getNumericsTextSize());
        m_linesTicksSizeSpinBox->setValue(m_chartOverlaySet->getAxisLineThickness());
        m_paddingSizeSpinBox->setValue(m_chartAxis->getPaddingSize());
        
        const int32_t overlayCount = m_chartOverlaySet->getNumberOfDisplayedOverlays();
        int32_t selectedOverlayIndex = m_chartAxis->getLabelOverlayIndex(overlayCount);
        const int32_t comboBoxCount = m_axisLabelFromOverlayComboBox->count();
        if (overlayCount < comboBoxCount) {
            m_axisLabelFromOverlayComboBox->setMaxCount(overlayCount);
        }
        else if (overlayCount > comboBoxCount) {
            for (int32_t j = comboBoxCount; j < overlayCount; j++) {
                m_axisLabelFromOverlayComboBox->addItem("Layer " + AString::number(j + 1));
            }
        }
        
        if ((selectedOverlayIndex >= 0)
            && (selectedOverlayIndex < m_axisLabelFromOverlayComboBox->count())) {
            m_axisLabelFromOverlayComboBox->setCurrentIndex(selectedOverlayIndex);
        }
        setEnabled(true);
    }
    else {
        setEnabled(false);
    }
    
    m_widgetGroup->blockAllSignals(false);
}

///**
// * Update the content with the given axis.
// *
// * @param chartOverlaySet
// *     Chart overlay set containing axis.
// * @param chartAxis
// *     New chart axis content.
// */
//void
//BrainBrowserWindowToolBarChartTwoAxes::updateControls(ChartTwoOverlaySet* chartOverlaySet,
//                                                      ChartTwoCartesianAxis* chartAxis)
//{
//    m_chartAxis = chartAxis;
//    
//    if ((chartOverlaySet != NULL)
//        && (m_chartAxis != NULL)) {
//        m_widgetGroup->blockAllSignals(true);
//        m_axisDisplayedByUserCheckBox->setChecked(chartAxis->isDisplayedByUser());
//        m_autoUserRangeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(m_chartAxis->getScaleRangeMode());
//        float rangeMin(0.0), rangeMax(0.0);
//        m_chartAxis->getRange(rangeMin, rangeMax);
//        m_userMinimumValueSpinBox->setRange(rangeMin, rangeMax);
//        m_userMinimumValueSpinBox->setValue(m_chartAxis->getUserScaleMinimumValue());
//        m_userMaximumValueSpinBox->setRange(rangeMin, rangeMax);
//        m_userMaximumValueSpinBox->setValue(m_chartAxis->getUserScaleMaximumValue());
//        m_showTickMarksCheckBox->setChecked(m_chartAxis->isShowTickmarks());
//        m_showLabelCheckBox->setChecked(m_chartAxis->isShowLabel());
//        const NumericFormatModeEnum::Enum numericFormat = m_chartAxis->getUserNumericFormat();
//        m_userNumericFormatComboBox->setSelectedItem<NumericFormatModeEnum, NumericFormatModeEnum::Enum>(numericFormat);
//        m_userDigitsRightOfDecimalSpinBox->setValue(m_chartAxis->getUserDigitsRightOfDecimal());
//        m_userDigitsRightOfDecimalSpinBox->setEnabled(numericFormat != NumericFormatModeEnum::AUTO);
//        m_numericSubdivisionsModeComboBox->setSelectedItem<ChartTwoNumericSubdivisionsModeEnum, ChartTwoNumericSubdivisionsModeEnum::Enum>(m_chartAxis->getNumericSubdivsionsMode());
//        m_userSubdivisionsSpinBox->setValue(m_chartAxis->getUserNumberOfSubdivisions());
//        m_userSubdivisionsSpinBox->setEnabled( m_chartAxis->getNumericSubdivsionsMode() == ChartTwoNumericSubdivisionsModeEnum::USER);
//        
//        const int32_t overlayCount = chartOverlaySet->getNumberOfDisplayedOverlays();
//        int32_t selectedOverlayIndex = m_chartAxis->getLabelOverlayIndex(overlayCount);
//        const int32_t comboBoxCount = m_axisLabelFromOverlayComboBox->count();
//        if (overlayCount < comboBoxCount) {
//            m_axisLabelFromOverlayComboBox->setMaxCount(overlayCount);
//        }
//        else if (overlayCount > comboBoxCount) {
//            for (int32_t j = comboBoxCount; j < overlayCount; j++) {
//                m_axisLabelFromOverlayComboBox->addItem("Overlay " + AString::number(j + 1));
//            }
//        }
//        
//        if ((selectedOverlayIndex >= 0)
//            && (selectedOverlayIndex < m_axisLabelFromOverlayComboBox->count())) {
//            m_axisLabelFromOverlayComboBox->setCurrentIndex(selectedOverlayIndex);
//        }
//        
//        m_widgetGroup->blockAllSignals(false);
//    }
//}

/**
 * Called when a widget is changed by the user.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::valueChanged()
{
    CaretAssert(m_chartAxis);
    if (m_chartAxis != NULL) {
        m_chartAxis->setLabelOverlayIndex(m_axisLabelFromOverlayComboBox->currentIndex());
        m_chartAxis->setDisplayedByUser(m_axisDisplayedByUserCheckBox->isChecked());
        m_chartAxis->setScaleRangeMode(m_autoUserRangeComboBox->getSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>());
        m_chartAxis->setUserScaleMinimumValue(m_userMinimumValueSpinBox->value());
        m_chartAxis->setUserScaleMaximumValue(m_userMaximumValueSpinBox->value());
        m_chartAxis->setShowTickmarks(m_showTickMarksCheckBox->isChecked());
        m_chartAxis->setShowLabel(m_showLabelCheckBox->isChecked());
        m_chartAxis->setUserNumericFormat(m_userNumericFormatComboBox->getSelectedItem<NumericFormatModeEnum, NumericFormatModeEnum::Enum>());
        m_chartAxis->setUserDigitsRightOfDecimal(m_userDigitsRightOfDecimalSpinBox->value());
        m_chartAxis->setNumericSubdivsionsMode(m_numericSubdivisionsModeComboBox->getSelectedItem<ChartTwoNumericSubdivisionsModeEnum, ChartTwoNumericSubdivisionsModeEnum::Enum>());
        m_chartAxis->setUserNumberOfSubdivisions(m_userSubdivisionsSpinBox->value());
        
        m_chartAxis->setLabelTextSize(m_labelSizeSpinBox->value());
        m_chartOverlaySet->setAxisLineThickness(m_linesTicksSizeSpinBox->value());
        m_chartAxis->setNumericsTextSize(m_numericsSizeSpinBox->value());
        m_chartAxis->setPaddingSize(m_paddingSizeSpinBox->value());
        
        const BrowserTabContent* tabContent = getTabContentFromSelectedTab();
        CaretAssert(tabContent);
        
        const YokingGroupEnum::Enum yokingGroup = tabContent->getYokingGroup();
        if (yokingGroup != YokingGroupEnum::YOKING_GROUP_OFF) {
            const ModelChartTwo* modelChartTwo = tabContent->getDisplayedChartTwoModel();
            CaretAssert(modelChartTwo);
            const int32_t tabIndex = tabContent->getTabNumber();
            EventChartTwoAttributesChanged attributesEvent;
            attributesEvent.setCartesianAxisChanged(yokingGroup,
                                                    modelChartTwo->getSelectedChartTwoDataType(tabIndex),
                                                    m_chartAxis);
            EventManager::get()->sendEvent(attributesEvent.getPointer());
        }
    }

    updateGraphics();
    
    updateContent(getTabContentFromSelectedTab());
}

/**
 * Update the graphics.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::updateGraphics()
{
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}

/**
 * Called when the minimum value is changed.
 *
 * @param minimumValue
 *     New minimum value.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::axisMinimumValueChanged(double minimumValue)
{
    if (m_chartAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_autoUserRangeComboBox->getWidget()->blockSignals(true);
        m_autoUserRangeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::AXIS_DATA_RANGE_USER);
        m_autoUserRangeComboBox->getWidget()->blockSignals(false);
        
        /*
         * Ensure maximum value is always greater than or equal to minimum
         */
        if (minimumValue > m_userMaximumValueSpinBox->value()) {
            m_userMaximumValueSpinBox->blockSignals(true);
            m_userMaximumValueSpinBox->setValue(minimumValue);
            m_userMaximumValueSpinBox->blockSignals(false);
        }
        
        valueChanged();
    }
}

/**
 * Called when the maximum value is changed.
 *
 * @param maximumValue
 *     New maximum value.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::axisMaximumValueChanged(double maximumValue)
{
    if (m_chartAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_autoUserRangeComboBox->getWidget()->blockSignals(true);
        m_autoUserRangeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::AXIS_DATA_RANGE_USER);
        m_autoUserRangeComboBox->getWidget()->blockSignals(false);

        /*
         * Ensure minimum value is always less than or equal to maximum
         */
        if (maximumValue < m_userMinimumValueSpinBox->value()) {
            m_userMinimumValueSpinBox->blockSignals(true);
            m_userMinimumValueSpinBox->setValue(maximumValue);
            m_userMinimumValueSpinBox->blockSignals(false);
        }
        
        valueChanged();
    }
}

///**
// * Called when the label size value is changed.
// *
// * @param value
// *     New value.
// */
//void
//BrainBrowserWindowToolBarChartTwoAxes::labelSizeValueChanged(double value)
//{
//}
//
///**
// * Called when the numerics size value is changed.
// *
// * @param value
// *     New value.
// */
//void
//BrainBrowserWindowToolBarChartTwoAxes::numericsSizeValueChanged(double)
//{
//    
//}
//
///**
// * Called when the lines/ticks size value is changed.
// *
// * @param value
// *     New value.
// */
//void
//BrainBrowserWindowToolBarChartTwoAxes::linesTicksSizeValueChanged(double)
//{
//    
//}
//
///**
// * Called when the padding size value is changed.
// *
// * @param value
// *     New value.
// */
//void
//BrainBrowserWindowToolBarChartTwoAxes::paddingSizeValueChanged(double)
//{
//    
//}

/**
 * Called when a widget is changed by a slot using a bool parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::valueChangedBool(bool)
{
    valueChanged();
}

/**
 * Called when a widget is changed by a slot using a double parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::valueChangedDouble(double)
{
    valueChanged();
}

/**
 * Called when a widget is changed by a slot using a int parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::valueChangedInt(int)
{
    valueChanged();
}

/**
 * Called when name toolbutton is clicked to change axis label.
 */
void
BrainBrowserWindowToolBarChartTwoAxes::axisLabelToolButtonClicked(bool)
{
    ChartTwoOverlaySet* chartOverlaySet = NULL;
    std::vector<ChartAxisLocationEnum::Enum> validAxesLocations;
    ChartTwoCartesianAxis* selectedAxis = NULL;
    AnnotationPercentSizeText* axisLabel = NULL;
    
    getSelectionData(getTabContentFromSelectedTab(),
                     chartOverlaySet,
                     validAxesLocations,
                     selectedAxis,
                     axisLabel);
    
    if (axisLabel != NULL) {
        WuQDataEntryDialog newNameDialog("Axis Label",
                                         m_axisLabelToolButton);
        QLineEdit* lineEdit = newNameDialog.addLineEditWidget("Label");
        lineEdit->setText(axisLabel->getText());
        if (newNameDialog.exec() == WuQDataEntryDialog::Accepted) {
            const AString name = lineEdit->text().trimmed();
            axisLabel->setText(name);
            valueChanged();
        }
    }
}




