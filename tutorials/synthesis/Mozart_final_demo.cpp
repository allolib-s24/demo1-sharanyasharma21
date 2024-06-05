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

#define FFT_SIZE 4096

// using namespace gam;
using namespace al;

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
    //createInternalTriggerParameter("speed", 1, 0.25, 5);
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
          spectrum[k] = tanh(pow(stft.bin(k).real(), 0.8)); 
          //spectrum[k] = log(1 + fabs(stft.bin(k).real())); // log function
          //spectrum[k] = sqrt(stft.bin(k).real()); // square-root function
          //spectrum[k] = 1 / (1 + exp(-stft.bin(k).real())); // sigmoid function
          //spectrum[k] = sin(stft.bin(k).real()) * exp(stft.bin(k).real());


        }
      }
    }

    // Let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done())
      free();
  }

  // The graphics processing function
  void onProcess(Graphics &g) override
  {

    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    mMelodySpect.reset();

    for (int i = 0; i < FFT_SIZE / 10; i++) 
    {
      
      mMelodySpect.color(HSV(frequency / 500 - spectrum[i] * 5)); 
      mMelodySpect.vertex(i, spectrum[i], 0.0);
    }

  // Constant color of spectogram depending on RGB value
  // 1,0,0 Red
  // 1,1,0 Yellow
    // for (int i = 0; i < FFT_SIZE / 10; i++) 
    // {
    // // Set a fixed RGB color, for example, red
    // mMelodySpect.color(1.0, 1.0, 0.0); // 1.0 for red, 0.0 for green and blue
    // mMelodySpect.vertex(i, spectrum[i], 0.0);
    // }
    
    for (int i = -4; i <= 5; i++) 
    {
      g.meshColor();
      g.pushMatrix();
      // g.translate(-0.5f, 1, -10);
      
      g.translate(cos(static_cast<double>(frequency)), sin(static_cast<double>(frequency)), -4);
      //g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -12);
      //g.translate(i, 0 , -50);
      
      g.scale(50.0 /FFT_SIZE, 250, 1);
      // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
      
      g.lineWidth(1 + 50 * mEnvFollow.value() * 100); 
      g.draw(mMelodySpect);
      g.popMatrix();
    }
    

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
      float freqMult = 1.0)
  {
    auto *voice = synthManager.synth().getVoice<MelodySpect>();
    vector<VariantValue> params = vector<VariantValue>({amplitude, frequency * freqMult, attackTime, releaseTime, pan});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

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

  void playPiece(float freqMult = 1.0){

    playNote(0,0.101415,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(0.0844572,0.100449,0.3,432,0.403,0.35,0,freqMult);
    playNote(0.185554,0.0991774,0.3,407.754,0.403,0.35,0,freqMult);
    playNote(0.318353,0.09949,0.3,432,0.403,0.35,0,freqMult);
    playNote(0.485624,0.214973,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(0.918025,0.116667,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(1.00156,0.133723,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(1.13487,0.0996081,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(1.28504,0.099314,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(1.41834,0.249281,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(1.86855,0.0672926,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(1.95188,0.100352,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(2.05181,0.116305,0.3,610.94,0.403,0.35,0,freqMult);
    playNote(2.15171,0.0823131,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(2.35168,0.0830753,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(2.46862,0.0836027,0.3,864,0.403,0.35,0,freqMult);
    playNote(2.5518,0.116883,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(2.66828,0.0997201,0.3,864,0.403,0.35,0,freqMult);
    playNote(2.80151,0.0996876,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(2.91862,0.0992045,0.3,864,0.403,0.35,0,freqMult);
    playNote(3.00174,0.133107,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(3.13563,0.0992477,0.3,864,0.403,0.35,0,freqMult);
    playNote(3.3014,0.316721,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(3.78529,0.166468,0.3,864,0.403,0.35,0,freqMult);
    playNote(4.05193,0.116266,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(4.28512,0.0998299,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(4.51834,0.166672,0.3,864,0.403,0.35,0,freqMult);
    playNote(4.75256,0.132111,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(5.03484,0.116588,0.3,864,0.403,0.35,0,freqMult);
    playNote(5.21918,0.132169,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(5.4851,0.132888,0.3,864,0.403,0.35,0,freqMult);
    playNote(5.70183,0.115685,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(5.96853,0.149732,0.3,864,0.403,0.35,0,freqMult);
    playNote(6.20188,0.116295,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(6.45193,0.149744,0.3,864,0.403,0.35,0,freqMult);
    playNote(6.66965,0.131994,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(6.90186,0.166144,0.3,726.534,0.403,0.35,0,freqMult);
    playNote(7.13628,0.232057,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(7.65158,0.0998549,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(7.75207,0.0833574,0.3,432,0.403,0.35,0,freqMult);
    playNote(7.83501,0.0995956,0.3,407.754,0.403,0.35,0,freqMult);
    playNote(7.93519,0.116117,0.3,432,0.403,0.35,0,freqMult);
    playNote(8.13497,0.266336,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(8.58929,0.145428,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(8.6845,0.150483,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(8.86858,0.0828702,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(9.1183,0.299786,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(9.65158,0.0676202,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(9.7018,0.133954,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(9.83523,0.116401,0.3,610.94,0.403,0.35,0,freqMult);
    playNote(9.91853,0.116279,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(10.1516,0.0669017,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(10.2191,0.0991755,0.3,864,0.403,0.35,0,freqMult);
    playNote(10.3017,0.116079,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(10.4175,0.100702,0.3,864,0.403,0.35,0,freqMult);
    playNote(10.6015,0.0830738,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(10.7183,0.116205,0.3,864,0.403,0.35,0,freqMult);
    playNote(10.8185,0.116091,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(10.9183,0.0996991,0.3,864,0.403,0.35,0,freqMult);
    playNote(11.1348,0.216356,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(11.6184,0.149553,0.3,864,0.403,0.35,0,freqMult);
    playNote(11.8517,0.133105,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(12.1186,0.116165,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(12.3186,0.183007,0.3,864,0.403,0.35,0,freqMult);
    playNote(12.5675,0.134252,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(12.8684,0.133024,0.3,864,0.403,0.35,0,freqMult);
    playNote(13.1186,0.0827683,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(13.3517,0.149976,0.3,864,0.403,0.35,0,freqMult);
    playNote(13.5699,0.0981417,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(13.8347,0.133282,0.3,864,0.403,0.35,0,freqMult);
    playNote(14.085,0.0989407,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(14.3017,0.116261,0.3,864,0.403,0.35,0,freqMult);
    playNote(14.535,0.116268,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(14.7511,0.116794,0.3,726.534,0.403,0.35,0,freqMult);
    playNote(15.052,0.232816,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(15.5183,0.233058,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(15.752,0.182872,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(16.0023,0.0821327,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(16.2186,0.0812124,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(16.4683,0.149879,0.3,864,0.403,0.35,0,freqMult);
    playNote(16.6017,0.0992713,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(16.685,0.183053,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(16.7682,0.199978,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(16.8852,0.316358,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(17.4017,0.182364,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(17.6183,0.233026,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(17.8845,0.0669317,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(18.085,0.083179,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(18.3185,0.13294,0.3,864,0.403,0.35,0,freqMult);
    playNote(18.4185,0.133813,0.3,769.736,0.403,0.35,0,freqMult);
    playNote(18.5519,0.149516,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(18.6518,0.182786,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(18.7853,0.282947,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(19.318,0.266679,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(19.535,0.200099,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(19.7909,0.0937856,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(20.018,0.116657,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(20.185,0.0830821,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(20.3017,0.199969,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(20.4017,0.21656,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(20.5186,0.149314,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(20.7017,0.26643,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(21.2185,0.283002,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(21.4015,0.266685,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(21.7017,0.0999657,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(21.9351,0.116477,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(22.1339,0.0844514,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(22.2351,0.216557,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(22.3685,0.215997,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(22.4684,0.18406,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(22.6347,0.350054,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(23.2019,0.116301,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(23.3189,0.0992813,0.3,432,0.403,0.35,0,freqMult);
    playNote(23.4018,0.132935,0.3,407.754,0.403,0.35,0,freqMult);
    playNote(23.5353,0.0825459,0.3,432,0.403,0.35,0,freqMult);
    playNote(23.7347,0.250042,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(24.185,0.10027,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(24.2848,0.0998787,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(24.3522,0.250115,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(24.6019,0.0497313,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(24.735,0.216565,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(25.1854,0.0667833,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(25.2681,0.116322,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(25.3851,0.133205,0.3,610.94,0.403,0.35,0,freqMult);
    playNote(25.4852,0.0831023,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(25.6686,0.0666396,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(25.7518,0.11652,0.3,864,0.403,0.35,0,freqMult);
    playNote(25.8518,0.133513,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(25.9518,0.0997555,0.3,864,0.403,0.35,0,freqMult);
    playNote(26.1353,0.0995078,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(26.3685,0.116152,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(26.3186,0.233661,0.3,864,0.403,0.35,0,freqMult);
    playNote(26.6016,0.316687,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(27.1353,0.149573,0.3,864,0.403,0.35,0,freqMult);
    playNote(27.353,0.098396,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(27.6181,0.149955,0.3,1027.47,0.403,0.35,0,freqMult);
    playNote(27.8521,0.11587,0.3,969.807,0.403,0.35,0,freqMult);
    playNote(28.1352,0.18213,0.3,864,0.403,0.35,0,freqMult);
    playNote(28.387,0.114806,0.3,815.507,0.403,0.35,0,freqMult);
    playNote(28.6347,0.133542,0.3,864,0.403,0.35,0,freqMult);
    playNote(28.9016,0.199863,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(29.1352,0.116371,0.3,685.757,0.403,0.35,0,freqMult);
    playNote(29.4351,0.032753,0.3,647.269,0.403,0.35,0,freqMult);
    playNote(29.5025,0.23236,0.3,576.651,0.403,0.35,0,freqMult);
    playNote(29.8686,0.299205,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(30.7684,0.099568,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(30.836,0.148694,0.3,513.737,0.403,0.35,0,freqMult);
    playNote(30.9685,0.316301,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(31.4017,0.0683642,0.3,432,0.403,0.35,0,freqMult);
    playNote(31.4696,0.131759,0.3,484.904,0.403,0.35,0,freqMult);
    playNote(31.5851,0.632915,0.3,432,0.403,0.35,0,freqMult);
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