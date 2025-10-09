#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

NS_CREATE_SIMPLE_TEST(Math, Color)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Constructor empty")
  {
#if NS_ENABLED(NS_COMPILE_FOR_DEBUG)
    if (nsMath::SupportsNaN<nsMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      nsColor defCtor;
      NS_TEST_BOOL(nsMath::IsNaN(defCtor.r) && nsMath::IsNaN(defCtor.g) && nsMath::IsNaN(defCtor.b) && nsMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    nsColor* pDefCtor = ::new ((void*)&testBlock[0]) nsColor;
    NS_TEST_BOOL(pDefCtor->r == 1.0f && pDefCtor->g == 2.0f && pDefCtor->b == 3.0f && pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    NS_TEST_BOOL(sizeof(nsColor) == sizeof(float) * 4);
  }
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Constructor components")
  {
    nsColor init3F(0.5f, 0.6f, 0.7f);
    NS_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    nsColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    NS_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Constructor copy")
  {
    nsColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    nsColor copy(init4F);
    NS_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }

  {
    nsColor cornflowerBlue(nsColor(0.39f, 0.58f, 0.93f));

    NS_TEST_BLOCK(nsTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      NS_TEST_BOOL(
        pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      NS_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b &&
                   pConstFloats[3] == cornflowerBlue.a);
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "HSV conversion")
  {
    nsColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    NS_TEST_BOOL(normalizedColor.IsNormalized());
    nsColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    NS_TEST_BOOL(!notNormalizedColor0.IsNormalized());
    nsColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    NS_TEST_BOOL(!notNormalizedColor1.IsNormalized());
    nsColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    NS_TEST_BOOL(!notNormalizedColor2.IsNormalized());
    nsColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    NS_TEST_BOOL(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    const nsColorGammaUB rgb[] = {nsColorGammaUB(255, 255, 255), nsColorGammaUB(0, 0, 0), nsColorGammaUB(123, 12, 1), nsColorGammaUB(31, 112, 153)};
    const nsVec3 hsv[] = {nsVec3(0, 0, 1), nsVec3(0, 0, 0), nsVec3(5.4f, 0.991f, 0.48f), nsVec3(200.2f, 0.797f, 0.600f)};

    for (int i = 0; i < 4; ++i)
    {
      const nsColor color = rgb[i];
      float hue, sat, val;
      color.GetHSV(hue, sat, val);

      NS_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      NS_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      NS_TEST_FLOAT(val, hsv[i].z, 0.1f);

      nsColor fromHSV = nsColor::MakeHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      NS_TEST_FLOAT(fromHSV.r, color.r, 0.01f);
      NS_TEST_FLOAT(fromHSV.g, color.g, 0.01f);
      NS_TEST_FLOAT(fromHSV.b, color.b, 0.01f);
    }
  }

  {
    if (nsMath::SupportsNaN<nsMathTestType>())
    {
      float fNaN = nsMath::NaN<float>();
      const nsColor nanArray[4] = {
        nsColor(fNaN, 0.0f, 0.0f, 0.0f), nsColor(0.0f, fNaN, 0.0f, 0.0f), nsColor(0.0f, 0.0f, fNaN, 0.0f), nsColor(0.0f, 0.0f, 0.0f, fNaN)};
      const nsColor compArray[4] = {
        nsColor(1.0f, 0.0f, 0.0f, 0.0f), nsColor(0.0f, 1.0f, 0.0f, 0.0f), nsColor(0.0f, 0.0f, 1.0f, 0.0f), nsColor(0.0f, 0.0f, 0.0f, 1.0f)};


      NS_TEST_BLOCK(nsTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 4; ++i)
        {
          NS_TEST_BOOL(nanArray[i].IsNaN());
          NS_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      NS_TEST_BLOCK(nsTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          NS_TEST_BOOL(!nanArray[i].IsValid());
          NS_TEST_BOOL(compArray[i].IsValid());

          NS_TEST_BOOL(!(compArray[i] * nsMath::Infinity<float>()).IsValid());
          NS_TEST_BOOL(!(compArray[i] * -nsMath::Infinity<float>()).IsValid());
        }
      }
    }
  }

  {
    const nsColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const nsColor op2(2.0, 0.3f, 0.0f, 1.0f);
    const nsColor compArray[4] = {
      nsColor(1.0f, 0.0f, 0.0f, 0.0f), nsColor(0.0f, 1.0f, 0.0f, 0.0f), nsColor(0.0f, 0.0f, 1.0f, 0.0f), nsColor(0.0f, 0.0f, 0.0f, 1.0f)};

    NS_TEST_BLOCK(nsTestBlock::Enabled, "SetRGB / SetRGBA")
    {
      nsColor c1(0, 0, 0, 0);

      c1.SetRGBA(1, 2, 3, 4);

      NS_TEST_BOOL(c1 == nsColor(1, 2, 3, 4));

      c1.SetRGB(5, 6, 7);

      NS_TEST_BOOL(c1 == nsColor(5, 6, 7, 4));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "IsIdenticalRGB")
    {
      nsColor c1(0, 0, 0, 0);
      nsColor c2(0, 0, 0, 1);

      NS_TEST_BOOL(c1.IsIdenticalRGB(c2));
      NS_TEST_BOOL(!c1.IsIdenticalRGBA(c2));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "IsIdenticalRGBA")
    {
      NS_TEST_BOOL(op1.IsIdenticalRGBA(op1));
      for (int i = 0; i < 4; ++i)
      {
        NS_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + nsMath::SmallEpsilon<float>() * compArray[i]));
        NS_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - nsMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "IsEqualRGB")
    {
      nsColor c1(0, 0, 0, 0);
      nsColor c2(0, 0, 0.2f, 1);

      NS_TEST_BOOL(!c1.IsEqualRGB(c2, 0.1f));
      NS_TEST_BOOL(c1.IsEqualRGB(c2, 0.3f));
      NS_TEST_BOOL(!c1.IsEqualRGBA(c2, 0.3f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "IsEqualRGBA")
    {
      NS_TEST_BOOL(op1.IsEqualRGBA(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        NS_TEST_BOOL(op1.IsEqualRGBA(op1 + nsMath::SmallEpsilon<float>() * compArray[i], 2 * nsMath::SmallEpsilon<float>()));
        NS_TEST_BOOL(op1.IsEqualRGBA(op1 - nsMath::SmallEpsilon<float>() * compArray[i], 2 * nsMath::SmallEpsilon<float>()));
        NS_TEST_BOOL(op1.IsEqualRGBA(op1 + nsMath::DefaultEpsilon<float>() * compArray[i], 2 * nsMath::DefaultEpsilon<float>()));
        NS_TEST_BOOL(op1.IsEqualRGBA(op1 - nsMath::DefaultEpsilon<float>() * compArray[i], 2 * nsMath::DefaultEpsilon<float>()));
      }
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator+= (nsColor)")
    {
      nsColor plusAssign = op1;
      plusAssign += op2;
      NS_TEST_BOOL(plusAssign.IsEqualRGBA(nsColor(-2.0f, 0.5f, -7.0f, 1.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator-= (nsColor)")
    {
      nsColor minusAssign = op1;
      minusAssign -= op2;
      NS_TEST_BOOL(minusAssign.IsEqualRGBA(nsColor(-6.0f, -0.1f, -7.0f, -1.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ooperator*= (float)")
    {
      nsColor mulFloat = op1;
      mulFloat *= 2.0f;
      NS_TEST_BOOL(mulFloat.IsEqualRGBA(nsColor(-8.0f, 0.4f, -14.0f, -0.0f), nsMath::SmallEpsilon<float>()));
      mulFloat *= 0.0f;
      NS_TEST_BOOL(mulFloat.IsEqualRGBA(nsColor(0.0f, 0.0f, 0.0f, 0.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator/= (float)")
    {
      nsColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      NS_TEST_BOOL(vDivFloat.IsEqualRGBA(nsColor(-2.0f, 0.1f, -3.5f, -0.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator+ (nsColor, nsColor)")
    {
      nsColor plus = (op1 + op2);
      NS_TEST_BOOL(plus.IsEqualRGBA(nsColor(-2.0f, 0.5f, -7.0f, 1.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator- (nsColor, nsColor)")
    {
      nsColor minus = (op1 - op2);
      NS_TEST_BOOL(minus.IsEqualRGBA(nsColor(-6.0f, -0.1f, -7.0f, -1.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator* (float, nsColor)")
    {
      nsColor mulFloatVec4 = 2 * op1;
      NS_TEST_BOOL(mulFloatVec4.IsEqualRGBA(nsColor(-8.0f, 0.4f, -14.0f, -0.0f), nsMath::SmallEpsilon<float>()));
      mulFloatVec4 = ((float)0 * op1);
      NS_TEST_BOOL(mulFloatVec4.IsEqualRGBA(nsColor(0.0f, 0.0f, 0.0f, 0.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator* (nsColor, float)")
    {
      nsColor mulVec4Float = op1 * 2;
      NS_TEST_BOOL(mulVec4Float.IsEqualRGBA(nsColor(-8.0f, 0.4f, -14.0f, -0.0f), nsMath::SmallEpsilon<float>()));
      mulVec4Float = (op1 * (float)0);
      NS_TEST_BOOL(mulVec4Float.IsEqualRGBA(nsColor(0.0f, 0.0f, 0.0f, 0.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator/ (nsColor, float)")
    {
      nsColor vDivVec4Float = op1 / 2;
      NS_TEST_BOOL(vDivVec4Float.IsEqualRGBA(nsColor(-2.0f, 0.1f, -3.5f, -0.0f), nsMath::SmallEpsilon<float>()));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator== (nsColor, nsColor)")
    {
      NS_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        NS_TEST_BOOL(!(op1 == (op1 + nsMath::SmallEpsilon<float>() * compArray[i])));
        NS_TEST_BOOL(!(op1 == (op1 - nsMath::SmallEpsilon<float>() * compArray[i])));
      }
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator< (nsColor, nsColor)")
    {
      for (int i = 0; i < 4; ++i)
      {
        for (int j = 0; j < 4; ++j)
        {
          if (i == j)
          {
            NS_TEST_BOOL(!(compArray[i] < compArray[j]));
            NS_TEST_BOOL(!(compArray[j] < compArray[i]));
          }
          else if (i < j)
          {
            NS_TEST_BOOL(!(compArray[i] < compArray[j]));
            NS_TEST_BOOL(compArray[j] < compArray[i]);
          }
          else
          {
            NS_TEST_BOOL(!(compArray[j] < compArray[i]));
            NS_TEST_BOOL(compArray[i] < compArray[j]);
          }
        }
      }
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator!= (nsColor, nsColor)")
    {
      NS_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        NS_TEST_BOOL(op1 != (op1 + nsMath::SmallEpsilon<float>() * compArray[i]));
        NS_TEST_BOOL(op1 != (op1 - nsMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator= (nsColorLinearUB)")
    {
      nsColor c;
      nsColorLinearUB lin(50, 100, 150, 255);

      c = lin;

      NS_TEST_FLOAT(c.r, 50 / 255.0f, 0.001f);
      NS_TEST_FLOAT(c.g, 100 / 255.0f, 0.001f);
      NS_TEST_FLOAT(c.b, 150 / 255.0f, 0.001f);
      NS_TEST_FLOAT(c.a, 1.0f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator= (nsColorGammaUB) / constructor(nsColorGammaUB)")
    {
      nsColor c;
      nsColorGammaUB gamma(50, 100, 150, 255);

      c = gamma;
      nsColor c3 = gamma;

      NS_TEST_BOOL(c == c3);

      NS_TEST_FLOAT(c.r, 0.031f, 0.001f);
      NS_TEST_FLOAT(c.g, 0.127f, 0.001f);
      NS_TEST_FLOAT(c.b, 0.304f, 0.001f);
      NS_TEST_FLOAT(c.a, 1.0f, 0.001f);

      nsColorGammaUB c2 = c;

      NS_TEST_INT(c2.r, 50);
      NS_TEST_INT(c2.g, 100);
      NS_TEST_INT(c2.b, 150);
      NS_TEST_INT(c2.a, 255);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetInvertedColor")
    {
      const nsColor c1(0.1f, 0.3f, 0.7f, 0.9f);

      nsColor c2 = c1.GetInvertedColor();

      NS_TEST_BOOL(c2.IsEqualRGBA(nsColor(0.9f, 0.7f, 0.3f, 0.1f), 0.01f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetLuminance")
    {
      NS_TEST_FLOAT(nsColor::Black.GetLuminance(), 0.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::White.GetLuminance(), 1.0f, 0.001f);

      NS_TEST_FLOAT(nsColor(0.5f, 0.5f, 0.5f).GetLuminance(), 0.2126f * 0.5f + 0.7152f * 0.5f + 0.0722f * 0.5f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetComplementaryColor")
    {
      // black and white have no complementary colors, or rather, they are their own complementary colors, apparently
      NS_TEST_BOOL(nsColor::Black.GetComplementaryColor().IsEqualRGBA(nsColor::Black, 0.001f));
      NS_TEST_BOOL(nsColor::White.GetComplementaryColor().IsEqualRGBA(nsColor::White, 0.001f));

      NS_TEST_BOOL(nsColor::Red.GetComplementaryColor().IsEqualRGBA(nsColor::Cyan, 0.001f));
      NS_TEST_BOOL(nsColor::Lime.GetComplementaryColor().IsEqualRGBA(nsColor::Magenta, 0.001f));
      NS_TEST_BOOL(nsColor::Blue.GetComplementaryColor().IsEqualRGBA(nsColor::Yellow, 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetSaturation")
    {
      NS_TEST_FLOAT(nsColor::Black.GetSaturation(), 0.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::White.GetSaturation(), 0.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::Red.GetSaturation(), 1.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::Lime.GetSaturation(), 1.0f, 0.001f);
      ;
      NS_TEST_FLOAT(nsColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator * / *= (nsMat4)")
    {
      nsMat4 m;
      m.SetIdentity();
      m = nsMat4::MakeScaling(nsVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(nsVec3(0.1f, 0.2f, 0.3f));

      nsColor c1 = m * nsColor::White;

      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "CalcAverageRGB")
    {
      nsColor c1(0.6f, 0.3f, 0.9f, 0.5f);
      NS_TEST_FLOAT(c1.CalcAverageRGB(), (0.6f + 0.3f + 0.9f) / 3.0f, 0.001f);

      nsColor c2(1.0f, 1.0f, 1.0f, 0.0f);
      NS_TEST_FLOAT(c2.CalcAverageRGB(), 1.0f, 0.001f);

      nsColor c3(0.0f, 0.0f, 0.0f, 1.0f);
      NS_TEST_FLOAT(c3.CalcAverageRGB(), 0.0f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ScaleRGB")
    {
      nsColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGB(2.0f);
      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(1.0f, 1.2f, 1.4f, 0.8f), 0.001f));

      nsColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGB(0.5f);
      NS_TEST_BOOL(c2.IsEqualRGBA(nsColor(0.2f, 0.15f, 0.1f, 0.1f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ScaleRGBA")
    {
      nsColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGBA(2.0f);
      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(1.0f, 1.2f, 1.4f, 1.6f), 0.001f));

      nsColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGBA(0.5f);
      NS_TEST_BOOL(c2.IsEqualRGBA(nsColor(0.2f, 0.15f, 0.1f, 0.05f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ComputeHdrMultiplier")
    {
      // LDR colors should return 1.0
      nsColor ldr1(0.5f, 0.3f, 0.7f, 1.0f);
      NS_TEST_FLOAT(ldr1.ComputeHdrMultiplier(), 1.0f, 0.001f);

      nsColor ldr2(1.0f, 0.9f, 0.8f, 0.5f);
      NS_TEST_FLOAT(ldr2.ComputeHdrMultiplier(), 1.0f, 0.001f);

      // HDR colors should return the largest component
      nsColor hdr1(2.0f, 1.5f, 1.0f, 0.5f);
      NS_TEST_FLOAT(hdr1.ComputeHdrMultiplier(), 2.0f, 0.001f);

      nsColor hdr2(1.0f, 3.5f, 2.2f, 1.0f);
      NS_TEST_FLOAT(hdr2.ComputeHdrMultiplier(), 3.5f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ComputeHdrExposureValue")
    {
      // LDR colors should return 0
      nsColor ldr(0.5f, 0.3f, 0.7f, 1.0f);
      NS_TEST_FLOAT(ldr.ComputeHdrExposureValue(), 0.0f, 0.001f);

      // HDR colors should return log2 of the multiplier
      nsColor hdr1(2.0f, 1.0f, 1.0f, 0.5f);
      NS_TEST_FLOAT(hdr1.ComputeHdrExposureValue(), 1.0f, 0.001f); // log2(2) = 1

      nsColor hdr2(4.0f, 2.0f, 1.0f, 0.5f);
      NS_TEST_FLOAT(hdr2.ComputeHdrExposureValue(), 2.0f, 0.001f); // log2(4) = 2
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ApplyHdrExposureValue")
    {
      nsColor c1(0.5f, 0.25f, 0.125f, 1.0f);
      c1.ApplyHdrExposureValue(2.0f); // 2^2 = 4
      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(2.0f, 1.0f, 0.5f, 1.0f), 0.001f));

      nsColor c2(1.0f, 0.5f, 0.25f, 0.8f);
      c2.ApplyHdrExposureValue(-1.0f); // 2^-1 = 0.5
      NS_TEST_BOOL(c2.IsEqualRGBA(nsColor(0.5f, 0.25f, 0.125f, 0.8f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "NormalizeToLdrRange")
    {
      // HDR color should be normalized
      nsColor hdr(4.0f, 2.0f, 1.0f, 0.5f);
      hdr.NormalizeToLdrRange();
      NS_TEST_BOOL(hdr.IsEqualRGBA(nsColor(1.0f, 0.5f, 0.25f, 0.5f), 0.001f));

      // LDR color should remain unchanged
      nsColor ldr(0.8f, 0.6f, 0.4f, 1.0f);
      ldr.NormalizeToLdrRange();
      NS_TEST_BOOL(ldr.IsEqualRGBA(nsColor(0.8f, 0.6f, 0.4f, 1.0f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetDarker")
    {
      nsColor bright(0.8f, 0.6f, 0.4f, 1.0f);
      nsColor darker = bright.GetDarker(2.0f);

      // Should be darker (lower values) but same alpha
      NS_TEST_BOOL(darker.r < bright.r && darker.g < bright.g && darker.b < bright.b);
      NS_TEST_FLOAT(darker.a, bright.a, 0.001f);

      // Test default factor
      nsColor darker2 = bright.GetDarker();
      NS_TEST_BOOL(darker2.r < bright.r && darker2.g < bright.g && darker2.b < bright.b);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "WithAlpha")
    {
      nsColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      nsColor c2 = c1.WithAlpha(0.3f);

      NS_TEST_BOOL(c2.IsEqualRGBA(nsColor(0.5f, 0.6f, 0.7f, 0.3f), 0.001f));
      // Original should be unchanged
      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(0.5f, 0.6f, 0.7f, 0.8f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ToRGBA8")
    {
      nsColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      nsUInt32 rgba = c1.ToRGBA8();

      // R=255, G=128, B=64, A=0 -> 0xFF804000 (R in MSB, A in LSB)
      NS_TEST_INT((rgba >> 24) & 0xFF, 255); // R
      NS_TEST_INT((rgba >> 16) & 0xFF, 128); // G
      NS_TEST_INT((rgba >> 8) & 0xFF, 64);   // B
      NS_TEST_INT(rgba & 0xFF, 0);           // A
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "ToABGR8")
    {
      nsColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      nsUInt32 abgr = c1.ToABGR8();

      // A=0, B=64, G=128, R=255 -> 0x004080FF (A in MSB, R in LSB)
      NS_TEST_INT((abgr >> 24) & 0xFF, 0);  // A
      NS_TEST_INT((abgr >> 16) & 0xFF, 64); // B
      NS_TEST_INT((abgr >> 8) & 0xFF, 128); // G
      NS_TEST_INT(abgr & 0xFF, 255);        // R
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "Static factory functions")
    {
      // MakeNaN
      if (nsMath::SupportsNaN<float>())
      {
        nsColor nanColor = nsColor::MakeNaN();
        NS_TEST_BOOL(nanColor.IsNaN());
      }

      // MakeZero
      nsColor zeroColor = nsColor::MakeZero();
      NS_TEST_BOOL(zeroColor.IsEqualRGBA(nsColor(0.0f, 0.0f, 0.0f, 0.0f), 0.001f));

      // MakeRGBA
      nsColor rgbaColor = nsColor::MakeRGBA(0.1f, 0.2f, 0.3f, 0.4f);
      NS_TEST_BOOL(rgbaColor.IsEqualRGBA(nsColor(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));

      nsColor rgbColor = nsColor::MakeRGBA(0.5f, 0.6f, 0.7f);
      NS_TEST_BOOL(rgbColor.IsEqualRGBA(nsColor(0.5f, 0.6f, 0.7f, 1.0f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "GetAsVec4")
    {
      nsColor c1(0.1f, 0.2f, 0.3f, 0.4f);
      nsVec4 v1 = c1.GetAsVec4();
      NS_TEST_BOOL(v1.IsEqual(nsVec4(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "Gamma/Linear conversion functions")
    {
      // Test single float conversions
      float gamma = 0.5f;
      float linear = nsColor::GammaToLinear(gamma);
      float backToGamma = nsColor::LinearToGamma(linear);
      NS_TEST_FLOAT(backToGamma, gamma, 0.001f);

      // Test Vec3 conversions
      nsVec3 gammaVec(0.2f, 0.5f, 0.8f);
      nsVec3 linearVec = nsColor::GammaToLinear(gammaVec);
      nsVec3 backToGammaVec = nsColor::LinearToGamma(linearVec);
      NS_TEST_BOOL(backToGammaVec.IsEqual(gammaVec, 0.001f));

      // Test edge cases
      NS_TEST_FLOAT(nsColor::GammaToLinear(0.0f), 0.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::GammaToLinear(1.0f), 1.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::LinearToGamma(0.0f), 0.0f, 0.001f);
      NS_TEST_FLOAT(nsColor::LinearToGamma(1.0f), 1.0f, 0.001f);
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator*= (nsColor)")
    {
      nsColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      nsColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      c1 *= c2;
      NS_TEST_BOOL(c1.IsEqualRGBA(nsColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "operator* (nsColor, nsColor)")
    {
      nsColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      nsColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      nsColor result = c1 * c2;
      NS_TEST_BOOL(result.IsEqualRGBA(nsColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    NS_TEST_BLOCK(nsTestBlock::Enabled, "MakeFromKelvin")
    {
      // Test some known temperature points
      nsColor warm = nsColor::MakeFromKelvin(2700);     // Warm white (incandescent)
      nsColor daylight = nsColor::MakeFromKelvin(6500); // Daylight
      nsColor cool = nsColor::MakeFromKelvin(9000);     // Cool daylight

      // Warm should be more red/orange
      NS_TEST_BOOL(warm.r > warm.b);

      // Cool should be more blue
      NS_TEST_BOOL(cool.b > cool.r);

      // Alpha should always be 1
      NS_TEST_FLOAT(warm.a, 1.0f, 0.001f);
      NS_TEST_FLOAT(daylight.a, 1.0f, 0.001f);
      NS_TEST_FLOAT(cool.a, 1.0f, 0.001f);

      // Test reasonable temperature values - all should produce valid colors
      NS_TEST_BOOL(warm.IsValid());
      NS_TEST_BOOL(daylight.IsValid());
      NS_TEST_BOOL(cool.IsValid());
    }
  }
}
