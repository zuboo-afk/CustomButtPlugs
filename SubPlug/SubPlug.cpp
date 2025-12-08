#include "SubPlug.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

SubPlug::SubPlug(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT bounds = pGraphics->GetBounds();
    const IRECT innerBounds = bounds.GetPadded(-10.f);
    const IRECT sliderBounds = innerBounds.GetFromLeft(150).GetMidVPadded(100);
    const IRECT versionBounds = innerBounds.GetFromTRHC(300, 20);
    const IRECT titleBounds = innerBounds.GetCentredInside(200, 50);

    if (pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControlWithTag(kCtrlTagSlider)->SetTargetAndDrawRECTs(sliderBounds);
      pGraphics->GetControlWithTag(kCtrlTagTitle)->SetTargetAndDrawRECTs(titleBounds);
      pGraphics->GetControlWithTag(kCtrlTagVersionNumber)->SetTargetAndDrawRECTs(versionBounds);
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_LIGHT_GRAY);
    pGraphics->AttachControl(new IVSliderControl(sliderBounds, kParamGain), kCtrlTagSlider);
    pGraphics->AttachControl(new ITextControl(titleBounds, "SubPlug", IText(30)), kCtrlTagTitle);
    WDL_String buildInfoStr;
    GetBuildInfoStr(buildInfoStr, __DATE__, __TIME__);
    pGraphics->AttachControl(new ITextControl(versionBounds, buildInfoStr.Get(), DEFAULT_TEXT.WithAlign(EAlign::Far)), kCtrlTagVersionNumber);
  };
#endif
}

#if IPLUG_EDITOR
void SubPlug::OnParentWindowResize(int width, int height)
{
  if(GetUI())
    GetUI()->Resize(width, height, 1.f, false);
}
#endif

#if IPLUG_DSP
void SubPlug::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{ 
  const double SR = GetSampleRate();
  
  for (int s = 0; s < nFrames; s++) {
    // Update parameters
    mParams.squareTerm = GetParam(kSquareTerm)->Value();
    mParams.cubicTerm = GetParam(kCubicTerm)->Value();
    mParams.sqrtTerm = GetParam(kSqrtTerm)->Value();
    mParams.cbrtTerm = GetParam(kCbrtTerm)->Value();
    mParams.conjugateTerm = GetParam(kConjugateTerm)->Value();
    mParams.mix = GetParam(kMix)->Value();
    mParams.inputGain = GetParam(kInputGain)->Value();
    mParams.outputGain = GetParam(kOutputGain)->Value();
    
    for (int c = 0; c < NOutChansConnected(); c++) {
      double x = inputs[c][s] * mParams.inputGain;
      double y = 0.0;  // imaginary part (for mono, use 0)
      
      // For stereo, you could use different strategies:
      // Option 1: Process channels independently (simpler)
      // Option 2: Link channels as complex (as in full version above)
      
      // Independent channel processing (safer for real-time)
      double x2 = x * x;
      double x3 = x2 * x;
      
      // Apply nonlinearities (with safety checks)
      double sqrt_x = (x >= 0) ? std::sqrt(x) : -std::sqrt(-x);
      double cbrt_x = (x >= 0) ? std::cbrt(x) : -std::cbrt(-x);
      
      // Conjugate term for real signal is just the signal itself
      double conj_term = x;  // conjugate of real x is x
      
      // MZ polynomial result
      double result = 
        mParams.squareTerm * x2 +
        mParams.cubicTerm * x3 +
        mParams.sqrtTerm * sqrt_x +
        mParams.cbrtTerm * cbrt_x +
        mParams.conjugateTerm * conj_term;
      
      // Apply mix and gain
      double wet = result * mParams.outputGain;
      double dry = x;  // original input (post input gain)
      outputs[c][s] = mParams.mix * wet + (1.0 - mParams.mix) * dry;
    }
  }
}
#endif
