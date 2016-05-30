/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialProducer.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrivialProducer.h"

#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"


#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"

#include <vector>

struct vtkPVTrivialProducerInternal
{
  std::vector<double> TimeSteps;
  std::vector<int> AllProcessExtents;
};

vtkStandardNewMacro(vtkPVTrivialProducer);
//----------------------------------------------------------------------------
vtkPVTrivialProducer::vtkPVTrivialProducer()
{
  this->Internals = new vtkPVTrivialProducerInternal;
}

//----------------------------------------------------------------------------
vtkPVTrivialProducer::~vtkPVTrivialProducer()
{
  if(this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::SetOutput(vtkDataObject* output)
{
  this->Superclass::SetOutput(output);
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::SetOutput(vtkDataObject* output, double time)
{
  if( this->Internals->TimeSteps.empty() == false &&
      time <= this->Internals->TimeSteps.back() )
    {
    vtkWarningMacro("New time step is not after last time step.");
    }
  this->Internals->TimeSteps.push_back(time);

  this->Modified();
  this->SetOutput(output);
}

//----------------------------------------------------------------------------
int vtkPVTrivialProducerPieceToExtentThreadSafe(
  int *resultExtent, vtkDataObject* dataSet)
{
  // this is really only meant for topologically structured grids
  if (vtkImageData* id = vtkImageData::SafeDownCast(dataSet))
    {
    id->GetExtent(resultExtent);
    }
  else if (vtkStructuredGrid* sd = vtkStructuredGrid::SafeDownCast(dataSet))
    {
    sd->GetExtent(resultExtent);
    }
  else if (vtkRectilinearGrid* rd = vtkRectilinearGrid::SafeDownCast(dataSet))
    {
    rd->GetExtent(resultExtent);
    }
  else
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int
vtkPVTrivialProducer::ProcessRequest(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  if (!this->Superclass::ProcessRequest(request, inputVector, outputVector))
    {
    return 0;
    }

  vtkInformation* outputInfo = outputVector->GetInformationObject(0);

  if( request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()) &&
      outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) )
    {
    double uTime = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    if( this->Internals->TimeSteps.empty() )
      {
      //vtkWarningMacro("Requesting a time step when none is available");
      }
    else if(uTime != this->Internals->TimeSteps.back())
      {
      vtkWarningMacro("Requesting time " << uTime << " but only "
                      << this->Internals->TimeSteps.back()
                      << " is available");
      }
    outputInfo->Get(vtkDataObject::DATA_OBJECT())->GetInformation()->Set(
      vtkDataObject::DATA_TIME_STEP(), uTime);
    }

  if(this->Internals->TimeSteps.empty() == false)
    {
    // outputInfo->Set(
    //   vtkDataObject::DATA_TIME_STEP(), this->Internals->TimeSteps.back());
    outputInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                    &this->Internals->TimeSteps[0],
                    static_cast<int>(this->Internals->TimeSteps.size()) );
    double timeRange[2] = {this->Internals->TimeSteps[0],
                           this->Internals->TimeSteps.back()};
    outputInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }


  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}