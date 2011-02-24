/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef __elxSplineKernelTransform_H_
#define __elxSplineKernelTransform_H_

#include "itkKernelTransform2.h"
#include "itkElasticBodySplineKernelTransform2.h"
#include "itkElasticBodyReciprocalSplineKernelTransform2.h"
#include "itkThinPlateSplineKernelTransform2.h"
#include "itkThinPlateR2LogRSplineKernelTransform2.h"
#include "itkVolumeSplineKernelTransform2.h"
#include "elxIncludes.h"

namespace elastix
{
  using namespace itk;

  /**
   * \class SplineKernelTransform
   * \brief A transform based on the itk::KernelTransform2.
   *
   * This transform is a nonrigid transformation, based on
   * thin-plate-spline-like kernels.
   *
   * The ITK code for this class is largely based on code by
   * Rupert Brooks. For elastix a few modifications were made
   * (making the transform thread safe, and make it support the
   * AdvancedTransform framework).
   *
   * This nonrigid transformation model allows the user to place control points
   * at application-specific positions, unlike the BSplineTransform, which always
   * uses a regular grid of control points.
   *
   * NB: in order to use this class for registration, the -fp command line
   * argument is mandatory! It is used to place the fixed image landmarks.
   *
   * The parameters used in this class are:
   * \parameter Transform: Select this transform as follows:\n
   *    <tt>(%Transform "SplineKernelTransform")</tt>
   * \parameter SplineKernelType: Select the deformation model, which must
   * be one of { ThinPlateSpline, ThinPlateR2LogRSpline, VolumeSpline,
   * ElasticBodySpline, ElasticBodyReciprocalSpline). In 2D this option is
   * ignored and a ThinPlateSpline will always be used. \n
   *   example: <tt>(SplineKernelType "ElasticBodySpline")</tt>\n
   * Default: ThinPlateSpline. You cannot specify this parameter for each
   * resolution differently.
   * \parameter SplineRelaxationFactor: make the spline interpolating or
   * approximating. A value of 0.0 gives an interpolating transform. Higher
   * values result in approximating splines.\n
   *   example: <tt>(SplineRelaxationFactor 0.01 )</tt>\n
   * Default: 0.0. You cannot specify this parameter for each resolution differently.
   * \parameter SplinePoissonRatio: Set the poisson ratio for the
   * ElasticBodySpline and the ElastixBodyReciprocalSpline. For other
   * SplineKernelTypes this parameters is ignored.\n
   *   example: <tt>(SplinePoissonRatio 0.3 )</tt>\n
   * Default: 0.3. You cannot specify this parameter for each resolution differently.\n
   * Valid values are withing -1.0 and 0.5. 0.5 means incompressible.
   * Negative values are a bit odd, but possible. See Wikipedia on PoissonRatio.
   *
   * \commandlinearg -fp: a file specifying a set of points that will serve
   * as fixed image landmarks.\n
   *   example: <tt>-fp fixedImagePoints.txt</tt> \n
   *   The fixedImagePoints.txt file should be structured: first line should
   * be "index" or "point", depending if the user supplies voxel indices or
   * real world coordinates. The second line should be the number of points
   * that should be transformed. The third and following lines give the
   * indices or points. The same structure thus as used for transformix.\n
   * \commandlinearg -mp: an optional file specifying a set of points that will serve
   * as moving image landmarks, used to initialize the transformation.\n
   *   example: <tt>-mp movingImagePoints.txt</tt> \n
   *   The movingImagePoints.txt should be structured like the fixedImagePoints.txt.
   *  The moving landmarks should be corresponding to the fixed landmarks.
   *  If no file is provided, the transformation is initialized to be the identity,
   *  i.e. the moving landmarks are chosen identical to the fixed landmarks.
   *
   * \transformparameter Transform: Select this transform as follows:\n
   *    <tt>(%Transform "SplineKernelTransform")</tt>
   * \transformparameter SplineKernelType: Select the deformation model,
   * which must be one of { ThinPlateSpline, ThinPlateR2LogRSpline, VolumeSpline,
   * ElasticBodySpline, ElasticBodyReciprocalSpline). In 2D this option is
   * ignored and a ThinPlateSpline will always be used. \n
   *   example: <tt>(SplineKernelType "ElasticBodySpline")</tt>\n   *
   * \transformparameter SplineRelaxationFactor: make the spline interpolating
   * or approximating. A value of 0.0 gives an interpolating transform.
   * Higher values result in approximating splines.\n
   *   example: <tt>(SplineRelaxationFactor 0.01 )</tt>\n   *
   * \transformparameter SplinePoissonRatio: Set the Poisson ratio for the
   * ElasticBodySpline and the ElastixBodyReciprocalSpline. For other
   * SplineKernelTypes this parameters is ignored.\n
   *   example: <tt>(SplinePoissonRatio 0.3 )</tt>\n
   * Valid values are withing -1.0 and 0.5. 0.5 means incompressible.
   * Negative values are a bit odd, but possible. See Wikipedia on PoissonRatio.
   * \transformparameter FixedImageLandmarks: The landmark positions in the
   * fixed image, in world coordinates. Positions written as x1 y1 [z1] x2 y2 [z2] etc.\n
   *   example: <tt>(FixedImageLandmarks 10.0 11.0 12.0 4.0 4.0 4.0 6.0 6.0 6.0 )</tt>
   *
   * \ingroup Transforms
   */

template < class TElastix >
class SplineKernelTransform : public AdvancedCombinationTransform<
  ITK_TYPENAME elx::TransformBase<TElastix>::CoordRepType,
  elx::TransformBase<TElastix>::FixedImageDimension > ,
  public elx::TransformBase<TElastix>
{
public:

  /** Standard ITK-stuff. */
  typedef SplineKernelTransform                     Self;
  typedef AdvancedCombinationTransform<
    typename elx::TransformBase<TElastix>::CoordRepType,
    elx::TransformBase<TElastix>::FixedImageDimension >   Superclass1;
  typedef elx::TransformBase<TElastix>                    Superclass2;

  /** The ITK-class that provides most of the functionality, and
   * that is set as the "CurrentTransform" in the CombinationTransform.
   */
  typedef KernelTransform2<
    typename elx::TransformBase<TElastix>::CoordRepType,
    elx::TransformBase<TElastix>::FixedImageDimension >   KernelTransformType;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro( Self );

  /** Run-time type information (and related methods). */
  itkTypeMacro( SplineKernelTransform, AdvancedCombinationTransform );

  /** Name of this class.
   * Use this name in the parameter file to select this specific transform. \n
   * example: <tt>(Transform "SplineKernelTransform")</tt>\n
   */
  elxClassNameMacro( "SplineKernelTransform" );

  /** Dimension of the domain space. */
  itkStaticConstMacro( SpaceDimension, unsigned int, Superclass2::FixedImageDimension );

  /** Typedefs inherited from the superclass. */
  typedef typename Superclass1::ScalarType                ScalarType;
  typedef typename Superclass1::ParametersType            ParametersType;
  typedef typename Superclass1::JacobianType              JacobianType;
  typedef typename Superclass1::InputVectorType           InputVectorType;
  typedef typename Superclass1::OutputVectorType          OutputVectorType;
  typedef typename Superclass1::InputCovariantVectorType  InputCovariantVectorType;
  typedef typename Superclass1::OutputCovariantVectorType OutputCovariantVectorType;
  typedef typename Superclass1::InputVnlVectorType        InputVnlVectorType;
  typedef typename Superclass1::OutputVnlVectorType       OutputVnlVectorType;
  typedef typename Superclass1::InputPointType            InputPointType;
  typedef typename Superclass1::OutputPointType           OutputPointType;

  /** Typedef's from the TransformBase class. */
  typedef typename Superclass2::ElastixType               ElastixType;
  typedef typename Superclass2::ElastixPointer            ElastixPointer;
  typedef typename Superclass2::ConfigurationType         ConfigurationType;
  typedef typename Superclass2::ConfigurationPointer      ConfigurationPointer;
  typedef typename Superclass2::RegistrationType          RegistrationType;
  typedef typename Superclass2::RegistrationPointer       RegistrationPointer;
  typedef typename Superclass2::CoordRepType              CoordRepType;
  typedef typename Superclass2::FixedImageType            FixedImageType;
  typedef typename Superclass2::MovingImageType           MovingImageType;
  typedef typename Superclass2::ITKBaseType               ITKBaseType;
  typedef typename Superclass2::CombinationTransformType  CombinationTransformType;

  /** Extra typedefs */
  typedef typename KernelTransformType::Pointer           KernelTransformPointer;
  typedef typename KernelTransformType::PointSetType      PointSetType;
  typedef typename PointSetType::Pointer                  PointSetPointer;

  /** Execute stuff before everything else:
   * \li Check if -fp command line argument was given
   * \li Check if -mp command line argument was given
   */
  virtual int BeforeAll( void );

  /** Execute stuff before the actual registration:
   * \li Setup transform
   * \li Determine fixed image (source) landmarks
   * \li Determine moving image (target) landmarks
   * \li Call InitializeTransform.
   */
  virtual void BeforeRegistration( void );

  /** Function to read transform-parameters from a file. */
  virtual void ReadFromFile( void );

  /** Function to write transform-parameters to a file. */
  virtual void WriteToFile( const ParametersType & param ) const;

protected:

  /** The constructor. */
  SplineKernelTransform();
  /** The destructor. */
  virtual ~SplineKernelTransform() {};

  typedef ThinPlateSplineKernelTransform2<
    CoordRepType, itkGetStaticConstMacro(SpaceDimension) >   TPKernelTransformType;
  typedef ThinPlateR2LogRSplineKernelTransform2<
    CoordRepType, itkGetStaticConstMacro(SpaceDimension) >   TPRKernelTransformType;
  typedef VolumeSplineKernelTransform2<
    CoordRepType, itkGetStaticConstMacro(SpaceDimension) >   VKernelTransformType;
  typedef ElasticBodySplineKernelTransform2<
    CoordRepType, itkGetStaticConstMacro(SpaceDimension) >   EBKernelTransformType;
  typedef ElasticBodyReciprocalSplineKernelTransform2<
    CoordRepType, itkGetStaticConstMacro(SpaceDimension) >   EBRKernelTransformType;

  /** Create an instance of a kernel transform. Returns false if the
   * kernelType is unknown.
   */
  virtual bool SetKernelType( const std::string & kernelType );

  /** Read source landmarks from fp file
   * \li Try reading -fp file
   */
  virtual void DetermineSourceLandmarks( void );

  /** Read target landmarks from mp file or load identity.
   * \li Try reading -mp file
   * \li If no -mp file was given, place landmarks as identity.
   */
  virtual bool DetermineTargetLandmarks( void );

  /** General function to read all landmarks. */
  virtual void ReadLandmarkFile(
    const std::string & filename,
    PointSetPointer landmarkPointSet,
    const bool & landmarksInFixedImage );

  /** The itk kernel transform. */
  KernelTransformPointer m_KernelTransform;

private:

  /** The private constructor. */
  SplineKernelTransform( const Self& ); // purposely not implemented
  /** The private copy constructor. */
  void operator=( const Self& );              // purposely not implemented

  std::string m_SplineKernelType;

}; // end class SplineKernelTransform


} // end namespace elastix

#ifndef ITK_MANUAL_INSTANTIATION
#include "elxSplineKernelTransform.hxx"
#endif

#endif // end #ifndef __elxSplineKernelTransform_H_
