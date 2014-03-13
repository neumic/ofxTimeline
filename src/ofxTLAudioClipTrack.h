/**
 * ofxTimeline
 * openFrameworks graphical timeline addon
 *
 * Copyright (c) 2011-2012 James George
 * Development Supported by YCAM InterLab http://interlab.ycam.jp/en/
 * http://jamesgeorge.org + http://flightphase.com
 * http://github.com/obviousjim + http://github.com/flightphase
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#pragma once

#include "ofMain.h"
#include "ofxTLTrack.h"
#include "ofxTLClipTrack.h"
#include "ofOpenALSoundPlayer_TimelineAdditions.h"

class ofxTLAudioClip : public ofxTLClip {
   public:
      ofxTLAudioClip();
      virtual void play();
      virtual void stop();
      virtual void setPlayerPosition( long millis );
      virtual long getPlayerDuration( );
      virtual void clampedMove( long millisOffset, long lower, long upper);
      virtual void clampedDragStart( long millisOffset );
      virtual void clampedDragEnd( long millisOffset );
      virtual bool loadFile( string path );

      virtual void recomputePreview( ofLongRange previewRange);
      bool shouldRecomputePreview;
      vector<ofPolyline> previews;

      bool fileLoaded;
   protected:
      float lastFFTPosition;
      int defaultSpectrumBandwidth;
      ofOpenALSoundPlayer_TimelineAdditions player;
};

class ofxTLAudioClipTrack : public ofxTLClipTrack {
  public:
	ofxTLAudioClipTrack();
	virtual ~ofxTLAudioClipTrack();

	virtual void drawClip(ofxTLClip* clip);

	virtual string getTrackType();

   virtual ofxTLClip* newClip();

  protected:
};
