#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// \brief Base class for image filtering functions used in scaling and resampling operations.
///
/// Image filters define how pixels are weighted when scaling images up or down.
/// Different filters provide different trade-offs between sharpness, aliasing, and ringing artifacts.
class NS_TEXTURE_DLL nsImageFilter
{
public:
  /// \brief Evaluates the filter function at the given distance from the center.
  ///
  /// The returned value represents the weight to apply to a sample at this distance.
  /// The function should return 0 for distances beyond the filter width.
  /// Note: The distribution may not be normalized - normalization is handled by the caller.
  virtual nsSimdFloat SamplePoint(const nsSimdFloat& x) const = 0;

  /// \brief Returns the filter support width (radius).
  ///
  /// The filter function is guaranteed to return 0 for |x| > width.
  /// Larger widths generally mean higher quality but slower filtering.
  nsSimdFloat GetWidth() const;

protected:
  nsImageFilter(float width);

private:
  nsSimdFloat m_fWidth;
};

/// \brief Box filter - fastest, produces blocky results.
///
/// The box filter provides uniform weighting within its support width.
/// Best used for pixel art or when nearest-neighbor-like behavior is desired.
/// Produces sharp edges but can create blocking artifacts.
class NS_TEXTURE_DLL nsImageFilterBox : public nsImageFilter
{
public:
  /// \param fWidth Filter support width, typically 0.5 for standard box filtering
  nsImageFilterBox(float fWidth = 0.5f);

  virtual nsSimdFloat SamplePoint(const nsSimdFloat& x) const override;
};

/// \brief Triangle (bilinear) filter - good balance of speed and quality.
///
/// The triangle filter provides linear weighting that falls to zero at the edges.
/// This is equivalent to bilinear interpolation and provides a good balance
/// between performance and visual quality with minimal ringing artifacts.
class NS_TEXTURE_DLL nsImageFilterTriangle : public nsImageFilter
{
public:
  /// \param fWidth Filter support width, typically 1.0 for standard triangle filtering
  nsImageFilterTriangle(float fWidth = 1.0f);

  virtual nsSimdFloat SamplePoint(const nsSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter - highest quality but may introduce ringing.
///
/// This filter provides the highest quality scaling with excellent preservation of detail.
/// The Kaiser window helps reduce ringing artifacts compared to an unwindowed sinc.
/// Use higher beta values for less ringing but more blurring.
///
/// **Parameter Guidelines:**
/// - Beta 2-4: Less ringing, more blurring
/// - Beta 4-6: Good balance (recommended range)
/// - Beta 6-8: Sharp but more ringing artifacts
class NS_TEXTURE_DLL nsImageFilterSincWithKaiserWindow : public nsImageFilter
{
public:
  /// \brief Constructs a Kaiser-windowed sinc filter.
  ///
  /// \param fWindowWidth Filter support width. Larger values provide higher quality but slower performance.
  ///                     Typical range: 2.0-4.0, with 3.0 being a good default.
  /// \param fBeta Kaiser window beta parameter controlling the trade-off between ringing and blurring.
  ///              This is alpha*pi in standard Kaiser window definitions. Range: 2.0-8.0, default 4.0.
  nsImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual nsSimdFloat SamplePoint(const nsSimdFloat& x) const override;

private:
  nsSimdFloat m_fBeta;
  nsSimdFloat m_fInvBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class NS_TEXTURE_DLL nsImageFilterWeights
{
public:
  /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
  nsImageFilterWeights(const nsImageFilter& filter, nsUInt32 uiSrcSamples, nsUInt32 uiDstSamples);

  /// \brief Returns the number of weights.
  nsUInt32 GetNumWeights() const;

  /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  nsSimdFloat GetWeight(nsUInt32 uiDstSampleIndex, nsUInt32 uiWeightIndex) const;

  /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline nsInt32 GetFirstSourceSampleIndex(nsUInt32 uiDstSampleIndex) const;

  nsArrayPtr<const float> ViewWeights() const;

private:
  nsHybridArray<float, 16> m_Weights;
  nsSimdFloat m_fWidthInSourceSpace;
  nsSimdFloat m_fSourceToDestScale;
  nsSimdFloat m_fDestToSourceScale;
  nsUInt32 m_uiNumWeights;
  nsUInt32 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
