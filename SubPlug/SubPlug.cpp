#include "SubPlug.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#endif

SubPlug::SubPlug(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

  // Phase rotation params
  GetParam(kSubOctave)->InitDouble("Sub Octave", 0.3, 0.0, 1.0, 0.01);
  GetParam(kPhaseShift)->InitDouble("Phase Shift", 0.5, 0.0, 1.0, 0.01);
  GetParam(kDrive)->InitDouble("Drive", 0.5, 0.0, 1.0, 0.01);
  GetParam(kMix)->InitDouble("Mix", 1.0, 0.0, 1.0, 0.01);
  
  // Initialize delay line (will be resized in OnReset)
  mDelayLine.resize(kMaxDelay);

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

//slider 

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
void SubPlug::OnReset()
{
  const double sampleRate = GetSampleRate();
  
  // Clear delay line
  std::fill(mDelayLine.begin(), mDelayLine.end(), 0.0f);
  mDelayPos = 0;
  mLastSubharmonic = 0.0f;
}

void SubPlug::OnParamChange(int paramIdx)
{
  switch (paramIdx) {
    case kSubOctave:
      mSubOctave = GetParam(kSubOctave)->Value();
      break;
    case kPhaseShift:
      mPhaseShift = GetParam(kPhaseShift)->Value();
      break;
    case kDrive:
      mDrive = GetParam(kDrive)->Value() * 10.0f + 1.0f;
      break;
    case kMix:
      mMix = GetParam(kMix)->Value();
      break;
  }
}

void SubPlug::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{ 
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    // Sum to mono (or process first channel only)
    sample input = inputs[0][s];
    
    // Create phase-shifted version using delay
    int delaySamples = static_cast<int>(mPhaseShift * 100.0f) + 1; // 1-101 samples
    sample delayed = ReadDelay(delaySamples);
    WriteDelay(input);
    
    // Generate subharmonic
    sample subharmonic = 0.0f;
    if (mSubOctave > 0.01f) {
      // Full-wave rectification for octave-down
      sample rectified = std::abs(input + delayed * 0.3f) * 2.0f - 1.0f;
      
      // Simple one-pole low-pass filter (smoothing)
      const float smoothCoeff = 0.1f; // Adjust for desired smoothness
      subharmonic = rectified * smoothCoeff + mLastSubharmonic * (1.0f - smoothCoeff);
      mLastSubharmonic = subharmonic;
      
      // Apply drive/waveshaping
      subharmonic = ApplyWaveShaper(subharmonic * mDrive);
      
      // Scale down the subharmonic
      subharmonic *= 0.7f;
    }
    
    // Mix with original
    sample processed = input * (1.0f - mSubOctave) + subharmonic * mSubOctave;
    
    // Apply dry/wet mix
    sample output = input * (1.0f - mMix) + processed * mMix;
    
    // Copy to all output channels
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = output;
    }
  }
  // const int nChans = NOutChansConnected();
  // const double gain = GetParam(kParamGain)->Value() / 100.;
  
  // for (int s = 0; s < nFrames; s++) {
  //   for (int c = 0; c < nChans; c++) {
  //     outputs[c][s] = inputs[c][s] * gain;
  //   }
  // }
}


void SubPlug::WriteDelay(sample s)
{
  mDelayLine[mDelayPos] = s;
  mDelayPos = (mDelayPos + 1) % kMaxDelay;
}

sample SubPlug::ReadDelay(int delaySamples)
{
  if (delaySamples <= 0 || delaySamples >= kMaxDelay) {
    return 0.0f;
  }
  
  int readPos = mDelayPos - delaySamples;
  if (readPos < 0) {
    readPos += kMaxDelay;
  }
  
  return mDelayLine[readPos];
}

sample SubPlug::ApplyWaveShaper(sample x)
{
  // Soft clipping tanh approximation
  // return std::tanh(x);
  
  // Alternative: cubic soft clipper (faster)
  if (x > 1.0f) return 0.6667f;
  if (x < -1.0f) return -0.6667f;
  return x - (x * x * x) / 3.0f;
  
  // Or use a hard clipper for more aggressive sound:
  // return std::max(-0.7f, std::min(0.7f, x));
}
#endif
};