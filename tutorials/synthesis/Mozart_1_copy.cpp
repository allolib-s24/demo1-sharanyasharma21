// Copied from 08_SubSyn.cpp

#include <cstdio> // for printing to stdout

#include <vector>
#include <random>
#include <iostream>
#include <map>

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/Spatial.h"
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

#include "al/app/al_App.hpp"
#include "al/io/al_Imgui.hpp"

using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

// using namespace gam;
using namespace al;

// This example shows how to use SynthVoice and SynthManagerto create an audio
// visual synthesizer. In a class that inherits from SynthVoice you will
// define the synth's voice parameters and the sound and graphic generation
// processes in the onProcess() functions.

class MelodySpect : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  Mesh mMelodySpect;
  vector<float> spectrum;
  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // addRect(mMesh, 1, 1, 0.5, 0.5);

    // Declare the size of the spectrum
    spectrum.resize(FFT_SIZE);
    mMelodySpect.primitive(Mesh::LINE_LOOP);
    // mMelodySpect.primitive(Mesh::POINTS);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 1.0, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    // createInternalTriggerParameter("Speed", )
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override
  {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    float f = getInternalParameterValue("frequency");
    mOsc.freq(f);
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      mEnvFollow(s1);
      io.out(0) += s1;
      io.out(1) += s2;

      if (stft(s1))
      {
        // loop through all frequency bins and scale the complex sample
        for (unsigned k = 0; k < stft.numBins(); ++k)
        {
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
        }
      }
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done())
      free();
  }

  // IMPLEMENT THIS FUNCTION FOR GRAPHICS!

  // The graphics processing function
  void onProcess(Graphics &g) override
  {

    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    mMelodySpect.reset();

    for (int i = 0; i < FFT_SIZE / 90; i++)
    {
      mMelodySpect.color(HSV(frequency / 500 - spectrum[i] * 50));
      mMelodySpect.vertex(i, spectrum[i], 0.0);
    }

    for (int i = 0; i <= 0; i++)
    {
      g.meshColor();
      g.pushMatrix();
      // g.translate(-0.5f, 1, -10);
      // g.translate(cos(static_cast<double>(frequency)), sin(static_cast<double>(frequency)), -4);
      g.translate(i, -2.7, -10);
      g.scale(50.0 / FFT_SIZE, 250, 1.0);
      // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
      g.lineWidth(1 + 5 * mEnvFollow.value() * 100);
      g.draw(mMelodySpect);
      g.popMatrix();
    }
    ///////

    // Calculate the number of samples to analyze
    // int numBins = 512; // Adjust the number of bins as needed
    // int numFrames = 64; // Adjust the number of frames as needed

    // Array to store audio data
    // float audioData[numBins];

    // // Read audio data from the audio input
    // float sample;
    // for (int i = 0; i < numBins; ++i) {
    //     if (io().inChannels() > 0) { // check whether there are available input channels in the audio input buffer.
    //         sample = io().in(0);
    //     } else {
    //         sample = 0.0f;
    //     }
    //     audioData[i] = sample;
    // }

    // // Clear the graphics buffer
    // g.clear();

    // // Set color for drawing the waveform
    // g.color(1.0f, 1.0f, 1.0f);

    // // Draw the waveform
    // g.begin(g.Lines);
    // for (int i = 0; i < numFrames; ++i) {
    //     float x = map(float(i), 0.0f, float(numFrames - 1), -1.0f, 1.0f);
    //     float y = audioData[i];
    //     g.vertex(x, y, -8.0f); // Draw vertex at (x, y, -8.0)
    // }
    // g.end();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override
  {
    mAmpEnv.reset();
  }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App
{
public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<MelodySpect> synthManager{"MelodySpect"};

  void playNote(
      float time,
      float duration,
      float amplitude,
      float frequency,
      float attackTime,
      float releaseTime,
      float pan,
      float pianoKeyX,
      float pianoKeyY,
      float pianoKeyWidth,
      float pianoKeyHeight,
      float freqMult = 1.0)
  {
    auto *voice = synthManager.synth().getVoice<MelodySpect>();
    vector<VariantValue> params = vector<VariantValue>({amplitude, frequency * freqMult, attackTime, releaseTime, pan, pianoKeyX, pianoKeyY, pianoKeyWidth, pianoKeyHeight});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  // Mesh and variables for drawing piano keys
  // Mesh meshKey;

  // float fontSize;
  //  Font renderder
  // FontRenderer fontRender;

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that shouƒld
  // happen once at startup.
  void onCreate() override
  {
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Set the font renderer
    // fontRender.load(Font::defaultFont().c_str(), 60, 1024);

    // Play example sequence. Comment this line to start from scratch
    // synthManager.synthSequencer().playSequence("rondo_2_5.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
  }

  void onAnimate(double dt) override
  {
    // The GUI is prepared here
    imguiBeginFrame();
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  // The graphics callback function.
  void onDraw(Graphics &g) override
  {
    g.clear();

    // This example uses only the orthogonal projection for 2D drawing
    // g.camera(Viewpoint::ORTHO_FOR_2D);  // Ortho [0:width] x [0:height]

    // Render the synth's graphics
    synthManager.render(g);

    // GUI is drawn here
    imguiDraw();
  }

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const &k) override
  {
    if (ParameterGUI::usingKeyboard())
    { // Ignore keys if GUI is using
      // keyboard
      return true;
    }
    if (k.key() == 'i')
    {
      playPiece();
      return true;
    }
    if (k.key() == 'o')
    {
      playPiece(0.5);
      return true;
    }
    if (k.shift())
    {
      // If shift pressed then keyboard sets preset
      int presetNumber = asciiToIndex(k.key());
      synthManager.recallPreset(presetNumber);
    }
    else
    {
      // Otherwise trigger note for polyphonic synth
      int midiNote = asciiToMIDI(k.key());

      if (midiNote > 0)
      {
        // Check which key is pressed

        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);

        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  // Whenever the window size changes this function is called
  void onResize(int w, int h) override
  {
    // Recaculate the size of graphic elements based new window size
  }

  void onExit() override { imguiShutdown(); }

  void playPiece(float freqMult = 1.0)
  {
    playNote(0, 0.12807, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(0.128002, 0.0533452, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
    playNote(0.170709, 0.138675, 0.268, 407.754, 0.735, 0.1, 0, 608.4, 174.75, 131.2, 174.75, freqMult);
    playNote(0.309326, 0.14932, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
    playNote(0.511987, 0.319997, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(1.02396, 0.138667, 0.268, 576.651, 0.735, 0.1, 0, 1083.6, 0, 131.2, 349.5, freqMult);
    playNote(1.152, 0.127943, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(1.22661, 0.096041, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(1.36531, 0.159939, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(1.56794, 0.351966, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(2.26125, 0.0426724, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(2.3466, 0.0960271, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(2.44257, 0.127976, 0.268, 610.94, 0.735, 0.1, 0, 202.8, 528.25, 131.2, 174.75, freqMult);
    playNote(2.51726, 0.191967, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(2.83722, 0.128154, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(2.9653, 0.127914, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(3.05055, 0.149319, 0.268, 815.507, 0.735, 0.1, 0, 608.4, 528.25, 131.2, 174.75, freqMult);
    playNote(3.18919, 0.149345, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(3.39185, 0.106742, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(3.49853, 0.128055, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(3.62652, 0.128082, 0.268, 815.507, 0.735, 0.1, 0, 608.4, 528.25, 131.2, 174.75, freqMult);
    playNote(3.75453, 0.138619, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(3.96786, 0.287982, 0.268, 1027.47, 0.735, 0.1, 0, 948.4, 353.5, 131.2, 349.5, freqMult);
    playNote(4.54382, 0.181281, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(4.77847, 0.191955, 0.268, 1027.47, 0.735, 0.1, 0, 948.4, 353.5, 131.2, 349.5, freqMult);
    playNote(5.09845, 0.127982, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(5.35444, 0.191991, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(5.62114, 0.181281, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(5.94109, 0.138663, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(6.17575, 0.181306, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(6.48507, 0.138661, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(6.75181, 0.181203, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(7.01838, 0.138636, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(7.25306, 0.127962, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(7.509, 0.191998, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(7.77568, 0.18129, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(8.09565, 0.181316, 0.268, 726.534, 0.735, 0.1, 0, 473.2, 528.25, 131.2, 174.75, freqMult);
    playNote(8.40498, 0.501277, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(8.90908, 0.342668, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(9.25127, 0.289268, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(9.54007, 0.183596, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(9.85536, 0.189238, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(10.0918, 0.134022, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(10.2254, 0.121832, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(10.3047, 0.140049, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(10.4897, 0.177507, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(10.6208, 0.445873, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(11.1618, 0.316099, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(11.4625, 0.183376, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(11.6465, 0.130046, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(11.9114, 0.181023, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(12.1715, 0.132263, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(12.3033, 0.0920965, 0.268, 769.736, 0.735, 0.1, 0, 542.8, 353.5, 131.2, 349.5, freqMult);
    playNote(12.3949, 0.117524, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(12.5912, 0.157597, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(12.6697, 0.367746, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(13.2214, 0.286036, 0.268, 513.737, 0.735, 0.1, 0, 2, 353.5, 131.2, 349.5, freqMult);
    playNote(13.458, 0.28928, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(13.7481, 0.130609, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(14.0098, 0.183816, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(14.2437, 0.0819653, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(14.3718, 0.217346, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(14.5109, 0.157168, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(14.5899, 0.209362, 0.268, 513.737, 0.735, 0.1, 0, 2, 353.5, 131.2, 349.5, freqMult);
    playNote(14.7617, 0.380041, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(15.3791, 0.280055, 0.268, 513.737, 0.735, 0.1, 0, 2, 353.5, 131.2, 349.5, freqMult);
    playNote(15.6158, 0.23686, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(15.852, 0.130435, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(16.1142, 0.146988, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(16.3509, 0.0780318, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(16.4756, 0.184336, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(16.6133, 0.17388, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(16.7446, 0.182985, 0.268, 513.737, 0.735, 0.1, 0, 2, 353.5, 131.2, 349.5, freqMult);
    playNote(16.9726, 0.255039, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(17.4949, 0.140334, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(17.5946, 0.122663, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
    playNote(17.7637, 0.138506, 0.268, 407.754, 0.735, 0.1, 0, 608.4, 174.75, 131.2, 174.75, freqMult);
    playNote(17.9018, 0.0781525, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
    playNote(17.9806, 0.315247, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(18.5328, 0.0781339, 0.268, 576.651, 0.735, 0.1, 0, 1083.6, 0, 131.2, 349.5, freqMult);
    playNote(18.5813, 0.0822402, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(18.628, 0.0997264, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(18.8218, 0.0463723, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(18.869, 0.0841609, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(19.0444, 0.408767, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(19.5006, 0.0838215, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(19.6641, 0.123386, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(19.7417, 0.152808, 0.268, 610.94, 0.735, 0.1, 0, 202.8, 528.25, 131.2, 174.75, freqMult);
    playNote(19.8742, 0.131368, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(20.0517, 0.0855764, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(20.1442, 0.171202, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(20.2682, 0.176027, 0.268, 815.507, 0.735, 0.1, 0, 608.4, 528.25, 131.2, 174.75, freqMult);
    playNote(20.3997, 0.131984, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(20.5312, 0.131227, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(20.6781, 0.116528, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(20.7941, 0.131941, 0.268, 815.507, 0.735, 0.1, 0, 608.4, 528.25, 131.2, 174.75, freqMult);
    playNote(20.9255, 0.0782663, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(21.0943, 0.304826, 0.268, 1027.47, 0.735, 0.1, 0, 948.4, 353.5, 131.2, 349.5, freqMult);
    playNote(21.6355, 0.130171, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(22.003, 0.289162, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(22.2928, 0.183227, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(22.5553, 0.183471, 0.268, 1027.47, 0.735, 0.1, 0, 948.4, 353.5, 131.2, 349.5, freqMult);
    playNote(22.9753, 0.39357, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(23.4479, 0.196129, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(23.7371, 0.291906, 0.268, 1027.47, 0.735, 0.1, 0, 948.4, 353.5, 131.2, 349.5, freqMult);
    playNote(24.0259, 0.267992, 0.268, 969.807, 0.735, 0.1, 0, 813.2, 353.5, 131.2, 349.5, freqMult);
    playNote(24.3406, 0.133027, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(24.6573, 0.130948, 0.268, 815.507, 0.735, 0.1, 0, 608.4, 528.25, 131.2, 174.75, freqMult);
    playNote(24.9775, 0.216678, 0.268, 864, 0.735, 0.1, 0, 678, 353.5, 131.2, 349.5, freqMult);
    playNote(25.2358, 0.293385, 0.268, 647.269, 0.735, 0.1, 0, 272.4, 353.5, 131.2, 349.5, freqMult);
    playNote(25.5253, 0.288749, 0.268, 685.757, 0.735, 0.1, 0, 407.6, 353.5, 131.2, 349.5, freqMult);
    playNote(25.8931, 0.250966, 0.268, 576.651, 0.735, 0.1, 0, 137.2, 353.5, 131.2, 349.5, freqMult);
    playNote(26.1822, 0.393968, 0.268, 513.737, 0.735, 0.1, 0, 2, 353.5, 131.2, 349.5, freqMult);
    playNote(26.7083, 0.085599, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(26.7944, 0.169861, 0.268, 513.737, 0.735, 0.1, 0, 948.4, 0, 131.2, 349.5, freqMult);
    playNote(26.9183, 0.28953, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(27.34, 0.0473198, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
    playNote(27.4184, 0.106245, 0.268, 484.904, 0.735, 0.1, 0, 813.2, 0, 131.2, 349.5, freqMult);
    playNote(27.5241, 0.604944, 0.268, 432, 0.735, 0.1, 0, 678, 0, 131.2, 349.5, freqMult);
  }
};

int main()
{
  // Create app instance
  MyApp app;

  // Set window size
  app.dimensions(1200, 600);

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
  return 0;
}
