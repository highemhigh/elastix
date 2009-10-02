/*======================================================================

  This file is part of the elastix software.

  Copyright (c) Erasmus MC, Rotterdam. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef __itkVarianceOverLastDimensionImageMetric_hxx
#define __itkVarianceOverLastDimensionImageMetric_hxx

#include "itkVarianceOverLastDimensionImageMetric.h"
#include "itkMersenneTwisterRandomVariateGenerator.h"
#include "vnl/algo/vnl_matrix_update.h"
#include <numeric>

namespace itk
{

  /**
  * ******************* Constructor *******************
  */

  template <class TFixedImage, class TMovingImage> 
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
      ::VarianceOverLastDimensionImageMetric():
        m_SampleLastDimensionRandomly( false ),
        m_NumSamplesLastDimension( 10 )
  {
    this->SetUseImageSampler( true );
    this->SetUseFixedImageLimiter( false );
    this->SetUseMovingImageLimiter( false );

  } // end constructor


  /**
   * ******************* Initialize *******************
   */

  template <class TFixedImage, class TMovingImage>
    void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::Initialize(void) throw ( ExceptionObject )
  {
    /** Initialize transform, interpolator, etc. */
    Superclass::Initialize();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int lastDimSize = this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );
    
    /** Check num last samples. */
    if ( this->m_NumSamplesLastDimension > lastDimSize ) {
      this->m_NumSamplesLastDimension = lastDimSize;
    }

  } // end Initialize


  /**
   * ******************* PrintSelf *******************
   */
  template < class TFixedImage, class TMovingImage> 
    void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::PrintSelf(std::ostream& os, Indent indent) const
  {
    Superclass::PrintSelf( os, indent );

  } // end PrintSelf

  /**
  * ******************* SampleRandom *******************
  */
  template < class TFixedImage, class TMovingImage> 
  void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::SampleRandom ( const int n, const int m, std::vector<int> & numbers ) const
  {
    /** Empty list of last dimension positions. */
    numbers.clear();

    /** Initialize random number generator. */
    Statistics::MersenneTwisterRandomVariateGenerator::Pointer randomGenerator = Statistics::MersenneTwisterRandomVariateGenerator::New();
    randomGenerator->SetSeed();

    /** Get n samples. */
    for ( int i = 0; i < n; ++i )
    {
      int randomNum = 0;
      do
      {
        randomNum = static_cast<int>( randomGenerator->GetVariateWithClosedRange( m ) );
      } while ( find( numbers.begin(), numbers.end(), randomNum ) != numbers.end() );
      numbers.push_back( randomNum );
    }
  } // end SampleRandom

  /**
   * *************** EvaluateTransformJacobianInnerProduct ****************
   */

  template < class TFixedImage, class TMovingImage >
    void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::EvaluateTransformJacobianInnerProduct( 
    const TransformJacobianType & jacobian, 
    const MovingImageDerivativeType & movingImageDerivative,
    DerivativeType & imageJacobian ) const
  {
    typedef typename TransformJacobianType::const_iterator JacobianIteratorType;
    typedef typename DerivativeType::iterator              DerivativeIteratorType;
    JacobianIteratorType jac = jacobian.begin();
    imageJacobian.Fill( 0.0 );
    const unsigned int sizeImageJacobian = imageJacobian.GetSize();
    for ( unsigned int dim = 0; dim < FixedImageDimension; dim++ )
    {
      const double imDeriv = movingImageDerivative[ dim ];
      DerivativeIteratorType imjac = imageJacobian.begin();
            
      for ( unsigned int mu = 0; mu < sizeImageJacobian; mu++ )
      {
        (*imjac) += (*jac) * imDeriv;
        ++imjac;
        ++jac;
      }
    }
  } // end EvaluateTransformJacobianInnerProduct


  /**
   * ******************* GetValue *******************
   */

  template <class TFixedImage, class TMovingImage> 
    typename VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>::MeasureType
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::GetValue( const TransformParametersType & parameters ) const
  {
    itkDebugMacro( "GetValue( " << parameters << " ) " );
    
    /** Initialize some variables */
    this->m_NumberOfPixelsCounted = 0;
    MeasureType measure = NumericTraits< MeasureType >::Zero;

    /** Make sure the transform parameters are up to date. */
    this->SetTransformParameters( parameters );

    /** Update the imageSampler and get a handle to the sample container. */
    this->GetImageSampler()->Update();
    ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

    /** Create iterator over the sample container. */
    typename ImageSampleContainerType::ConstIterator fiter;
    typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
    typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int lastDimSize = this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );
    const unsigned int numLastDimSamples = this->m_NumSamplesLastDimension;

    /** Vector containing last dimension positions to use: initialize on all positions. */
    std::vector<int> lastDimPositions (lastDimSize);
    for ( unsigned int i = 0; i < lastDimSize; ++i ) 
    {
      lastDimPositions[i] = i;
    }

    /** Loop over the fixed image samples to calculate the variance over time for every sample position. */
    for ( fiter = fbegin; fiter != fend; ++fiter )
    {
      /** Read fixed coordinates. */
      FixedImagePointType fixedPoint = (*fiter).Value().m_ImageCoordinates;

      /** Determine random last dimension positions if needed. */
      if ( this->m_SampleLastDimensionRandomly ) {
        SampleRandom( numLastDimSamples, lastDimSize, lastDimPositions );
      }

      /** Transform sampled point to voxel coordinates. */
      FixedImageContinuousIndexType voxelCoord;
      this->GetFixedImage()->TransformPhysicalPointToContinuousIndex( fixedPoint, voxelCoord );

      /** Loop over the slowest varying dimension. */
      double sumValues = 0.0;
      double sumValuesSquared = 0.0;
      unsigned int numSamplesOk = 0;
      const unsigned int realNumLastDimPositions = lastDimPositions.size();
      for (unsigned int d = 0; d < realNumLastDimPositions; ++d)
      {
        /** Initialize some variables. */
        RealType movingImageValue;
        MovingImagePointType mappedPoint;

        /** Set fixed point's last dimension to lastDimPosition. */
        voxelCoord[ lastDim ] = lastDimPositions[ d ];

        /** Transform sampled point back to world coordinates. */
        this->GetFixedImage()->TransformContinuousIndexToPhysicalPoint( voxelCoord, fixedPoint );
        
        /** Transform point and check if it is inside the bspline support region. */
        bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );

        /** Check if point is inside mask. */
        if ( sampleOk ) 
        {
          sampleOk = this->IsInsideMovingMask( mappedPoint );        
        }

        /** Compute the moving image value and check if the point is
        * inside the moving image buffer. */
        if ( sampleOk )
        {
          sampleOk = this->EvaluateMovingImageValueAndDerivative(
            mappedPoint, movingImageValue, 0 );
        }

        if ( sampleOk )
        {
          numSamplesOk++;
          sumValues += movingImageValue;
          sumValuesSquared += movingImageValue * movingImageValue;
        } // end if sampleOk
      } // end for loop over last dimension

      if ( numSamplesOk > 0 )
      {
        this->m_NumberOfPixelsCounted++;

        /** Add this variance to the variance sum. */
        const double expectedValue = sumValues / static_cast< double > ( numSamplesOk );
        const double expectedSquaredValue = sumValuesSquared / static_cast< double > ( numSamplesOk );
        measure += expectedSquaredValue - expectedValue * expectedValue;
      }

    } // end for loop over the image sample container

    /** Check if enough samples were valid. */
    this->CheckNumberOfSamples(
      sampleContainer->Size(), this->m_NumberOfPixelsCounted );

    /** Compute average over variances. */
    measure /= static_cast<double>( this->m_NumberOfPixelsCounted );

    /** Return the mean squares measure value. */
    return measure;

  } // end GetValue
  

  /**
   * ******************* GetDerivative *******************
   */

  template < class TFixedImage, class TMovingImage> 
    void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::GetDerivative( const TransformParametersType & parameters,
    DerivativeType & derivative ) const
  {
    /** When the derivative is calculated, all information for calculating
     * the metric value is available. It does not cost anything to calculate
     * the metric value now. Therefore, we have chosen to only implement the
     * GetValueAndDerivative(), supplying it with a dummy value variable. */
    MeasureType dummyvalue = NumericTraits< MeasureType >::Zero;
    this->GetValueAndDerivative( parameters, dummyvalue, derivative );

  } // end GetDerivative


  /**
   * ******************* GetValueAndDerivative *******************
   */

  template <class TFixedImage, class TMovingImage>
    void
    VarianceOverLastDimensionImageMetric<TFixedImage,TMovingImage>
    ::GetValueAndDerivative( const TransformParametersType & parameters, 
    MeasureType & value, DerivativeType & derivative ) const
  {
    itkDebugMacro("GetValueAndDerivative( " << parameters << " ) ");
    
    /** Define derivative and Jacobian types. */
    typedef typename DerivativeType::ValueType        DerivativeValueType;
    typedef typename TransformJacobianType::ValueType TransformJacobianValueType;

    /** Initialize some variables */
    this->m_NumberOfPixelsCounted = 0;
    MeasureType measure = NumericTraits< MeasureType >::Zero;
    derivative = DerivativeType( this->GetNumberOfParameters() );
    derivative.Fill( NumericTraits< DerivativeValueType >::Zero );
    
    /** Make sure the transform parameters are up to date. */
    this->SetTransformParameters( parameters );

    /** Update the imageSampler and get a handle to the sample container. */
    this->GetImageSampler()->Update();
    ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

    /** Create iterator over the sample container. */
    typename ImageSampleContainerType::ConstIterator fiter;
    typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
    typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int lastDimSize = 
      this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );
    const unsigned int numLastDimSamples = this->m_NumSamplesLastDimension;

    /** Vector containing last dimension positions to use: initialize on all positions. */
    std::vector<int> lastDimPositions ( lastDimSize );
    for ( unsigned int i = 0; i < lastDimSize; ++i ) 
    {
      lastDimPositions[ i ] = i;
    }

    /** Create variables to store intermediate results in. */
    TransformJacobianType jacobian;
    DerivativeType imageJacobian( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices() );      

    /** Get real last dim samples. */
    const unsigned int realNumLastDimPositions = this->m_SampleLastDimensionRandomly ? numLastDimSamples : lastDimSize;

    /** Variable to store and nzjis. */
    std::vector<NonZeroJacobianIndicesType> nzjis ( 
      realNumLastDimPositions, NonZeroJacobianIndicesType() );

    std::vector< RealType > MT ( realNumLastDimPositions );
    std::vector< DerivativeType > dMTdmu ( realNumLastDimPositions );

    /** Loop over the fixed image samples to calculate the variance over time for every sample position. */
    for ( fiter = fbegin; fiter != fend; ++fiter )
    {
      /** Read fixed coordinates. */
      FixedImagePointType fixedPoint = (*fiter).Value().m_ImageCoordinates;

      /** Determine random last dimension positions if needed. */
      if ( this->m_SampleLastDimensionRandomly ) 
      {
        SampleRandom( numLastDimSamples, lastDimSize, lastDimPositions );
      }

      /** Initialize MT vector. */
      std::fill(MT.begin(), MT.end(), itk::NumericTraits<RealType>::Zero);

      /** Transform sampled point to voxel coordinates. */
      FixedImageContinuousIndexType voxelCoord;
      this->GetFixedImage()->TransformPhysicalPointToContinuousIndex( fixedPoint, voxelCoord );

      /** Loop over the slowest varying dimension. */
      double sumValues = 0.0;
      double sumValuesSquared = 0.0;
      unsigned int numSamplesOk = 0;

      /** First loop over t: compute M(T(x,t)), dM(T(x,t))/dmu, nzji and store. */
      for ( unsigned int d = 0; d < realNumLastDimPositions; ++d ) 
      {
        /** Initialize some variables. */
        RealType movingImageValue;
        MovingImagePointType mappedPoint;
        MovingImageDerivativeType movingImageDerivative;

        /** Set fixed point's last dimension to lastDimPosition. */
        voxelCoord[ lastDim ] = lastDimPositions[ d ];
        /** Transform sampled point back to world coordinates. */
        this->GetFixedImage()->TransformContinuousIndexToPhysicalPoint( voxelCoord, fixedPoint );
        /** Transform point and check if it is inside the bspline support region. */
        bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );

        /** Check if point is inside mask. */
        if ( sampleOk )
        {
          sampleOk = this->IsInsideMovingMask( mappedPoint );        
        }

        /** Compute the moving image value and check if the point is
        * inside the moving image buffer. */
        if ( sampleOk )
        {
          sampleOk = this->EvaluateMovingImageValueAndDerivative(
            mappedPoint, movingImageValue, &movingImageDerivative );
        }

        if ( sampleOk )
        {
          /** Update value terms **/
          numSamplesOk++;
          sumValues += movingImageValue;
          sumValuesSquared += movingImageValue * movingImageValue;

          /** Get the TransformJacobian dT/dmu. */
          this->EvaluateTransformJacobian( fixedPoint, jacobian, nzjis[ d ] );

          /** Compute the innerproduct (dM/dx)^T (dT/dmu). */
          this->EvaluateTransformJacobianInnerProduct( 
            jacobian, movingImageDerivative, imageJacobian );

          /** Store values. */
          MT[ d ] = movingImageValue;
          dMTdmu[ d ] = imageJacobian;

        } 
        else 
        {
          dMTdmu[ d ] = DerivativeType( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices() );
          dMTdmu[ d ].Fill( itk::NumericTraits< DerivativeValueType >::Zero );
          nzjis[ d ] = NonZeroJacobianIndicesType( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices(), 0 );
        } // end if sampleOk
      }

      if ( numSamplesOk > 0 )
      {
        this->m_NumberOfPixelsCounted++;

        /** Compute average intensity value. */
        const double expectedValue = sumValues / static_cast< double > ( numSamplesOk );
        /** Add this variance to the variance sum. */
        const double expectedSquaredValue = sumValuesSquared / static_cast< double > ( numSamplesOk );
        measure += expectedSquaredValue - expectedValue * expectedValue;
        
        /** Second loop over t: update derivative. */
        for ( unsigned int d = 0; d < realNumLastDimPositions; ++d ) 
        {
          for ( unsigned int j = 0; j < nzjis[ d ].size(); ++j )
          {
            derivative[ nzjis[ d ][ j ] ] += ( 2.0 * ( MT[ d ] - expectedValue ) * dMTdmu[ d ][ j ] ) / static_cast< double > ( numSamplesOk );
          }
        }

      }
    } // end for loop over the image sample container
    
    /** Check if enough samples were valid. */
    this->CheckNumberOfSamples(
      sampleContainer->Size(), this->m_NumberOfPixelsCounted );
    
    /** Compute average over variances. */
    measure /= static_cast<double>( this->m_NumberOfPixelsCounted );
    derivative /= static_cast<double>( this->m_NumberOfPixelsCounted );

    /** Return the mean squares measure value. */
    value = measure;

  } // end GetValueAndDerivative()

} // end namespace itk

#endif // end #ifndef _itkVarianceOverLastDimensionImageMetric_hxx

