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

#include "ofxTLAudioClipTrack.h"
#include "ofxTimeline.h"
#include "ofxHotKeys.h"
#include "ofSystemUtils.h"

ofxTLAudioClip::ofxTLAudioClip() {
   fileLoaded = false;
   lastFFTPosition = -1;
   defaultSpectrumBandwidth = 1024;
}

void ofxTLAudioClip::play(){
   if( fileLoaded && !player.getIsPlaying() ){
      player.play();
   }
}

void ofxTLAudioClip::stop(){
   if( fileLoaded ){
      player.stop();
   }
}

void ofxTLAudioClip::setPosition( long millis ){
   if( fileLoaded ){
      player.setPositionMS( millis - timeRange.min );
   }
}

bool ofxTLAudioClip::loadFile( string path ){
   player.stop();
   if(player.loadSound(path, false)){
      ofxTLClip::loadFile( path );
      player.getSpectrum(defaultSpectrumBandwidth);
      player.setLogAverages(88, 20); //magic numbers defaults from audioTrack
      timeRange.setMax( timeRange.min + player.getDuration() *1000 );
      fileLoaded = true;
      movedSinceUpdate = true;
      return true;
   }
   movedSinceUpdate = true;
   return false;
}

ofxTLAudioClipTrack::ofxTLAudioClipTrack(){
}

ofxTLAudioClipTrack::~ofxTLAudioClipTrack(){
	
}

void ofxTLAudioClipTrack::draw(){
	
	//this is just a simple example
	ofPushStyle();
	ofNoFill();
	for(int i = 0; i < clips.size(); i++){
		float boxStart = millisToScreenX(clips[i] -> timeRange.min);
		float boxWidth = millisToScreenX(clips[i] -> timeRange.max) - millisToScreenX(clips[i] -> timeRange.min);
		if(boxStart + boxWidth > bounds.x && boxStart < bounds.x+bounds.width){
			//float screenY = ofMap(clickPoints[i].value, 0.0, 1.0, bounds.getMinY(), bounds.getMaxY());
			//ofCircle(screenX, bounds.getMinY() + 10, 4);
         if( clips[i] -> isSelected() ){
            ofSetColor(timeline->getColors().textColor);
         } else {
            ofSetColor(timeline->getColors().keyColor);
         }
         ofRect(boxStart, bounds.getMinY(), boxWidth, bounds.height );
		}
	}
}


string ofxTLAudioClipTrack::getTrackType(){
	return "AudioClipTrack";
}


ofxTLClip* ofxTLAudioClipTrack::newClip(){
   return new ofxTLAudioClip();
}

