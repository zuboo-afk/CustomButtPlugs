#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kParamGain = 0,
  kSubOctave,
  kPhaseShift,
  kDrive,
  kMix,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagVersionNumber = 0,
  kCtrlTagSlider,
  kCtrlTagTitle
};

using namespace iplug;
using namespace igraphics;

class SubPlug final : public Plugin
{
public:
  SubPlug(const InstanceInfo& info);

#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
#endif
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  
  static constexpr int kMaxDelay = 441; // ~10ms at 44.1kHz
  
  // Parameters
  float mSubOctave = 0.3f;
  float mPhaseShift = 0.5f;
  float mDrive = 5.5f;  // 0.5 * 10 + 1
  float mMix = 1.0f;
  
  // Processing state
  std::vector<sample> mDelayLine;
  int mDelayPos = 0;
  float mLastSubharmonic = 0.0f;
  
  // Helper functions
  void WriteDelay(sample s);
  sample ReadDelay(int delaySamples);
  sample ApplyWaveShaper(sample x);
#endif
};
