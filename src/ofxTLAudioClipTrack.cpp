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
   shouldRecomputePreview = true;
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

void ofxTLAudioClip::setPlayerPosition( long millis ){
   if( fileLoaded ){
      player.setPositionMS( millis );
   }
}

long ofxTLAudioClip::getPlayerDuration( ){
   return player.getDuration() * 1000;
}

void ofxTLAudioClip::clampedMove( long millisOffset, long lower, long upper){
   ofxTLClip::clampedMove( millisOffset, lower, upper);
   shouldRecomputePreview = true;
}

void ofxTLAudioClip::clampedDragStart( long millisOffset ){
   ofxTLClip::clampedDragStart( millis );
   shouldRecomputePreview = true;
}

void ofxTLAudioClip::clampedDragEnd( long millis ){
   ofxTLClip::clampedDragEnd( millis );
   shouldRecomputePreview = true;
}

bool ofxTLAudioClip::loadFile( string path ){
   player.stop();
   if(player.loadSound(path, false)){
      ofxTLClip::loadFile( path );
      player.getSpectrum(defaultSpectrumBandwidth);
      player.setLogAverages(88, 20); //magic numbers defaults from audioTrack
      timeRange.setMax( timeRange.min + getPlayerDuration() );
      fileLoaded = true;
      shouldRecomputePreview = true;
      movedSinceUpdate = true;
      return true;
   }
   movedSinceUpdate = true;
   return false;
}

void ofxTLAudioClip::recomputePreview(ofLongRange previewRange){
   previews.clear();
   int pixelWidth = displayRect.width;
   if( pixelWidth < 5 ){
      return;
   }

   //change this to be a less arbitrary range (0-1?)
   float trackHeight = 100/(1+player.getNumChannels());

   int numChannels = player.getNumChannels();
   vector<short> & buffer  = player.getBuffer();
   int numSamples = buffer.size() / numChannels;
   int frameSize = 
      (numSamples * timeRange.span() / previewRange.span()) / pixelWidth;
   int stepSize  = 2 + (frameSize / 60);//this number is magic. It seems about right

   //Range between 0.0-1.0 of the player that we are previewing
   ofFloatRange positionRange = ofFloatRange(
      ofMap( previewRange.min, timeRange.min + playerOffset, timeRange.min + getPlayerDuration() + playerOffset, 0.0, 1.0 ),
      ofMap( previewRange.max, timeRange.min + playerOffset, timeRange.min + getPlayerDuration() + playerOffset, 0.0, 1.0 ) );

   for(int c = 0; c < numChannels; c++){
      ofPolyline preview;
      int lastFrameIndex = 0;
      float trackCenter = trackHeight * (c+1);
      preview.resize(pixelWidth*2);
      for(int i = 0; i < pixelWidth; i++){
         float pointInTrack = ofMap( i, 0, pixelWidth, positionRange.min, positionRange.max );

         ofPoint * vertex = & preview.getVertices()[ i * 2 ];

         if(pointInTrack >= 0 && pointInTrack <= 1.0){
            int frameIndex = pointInTrack * numSamples;
            float losample = 0;
            float hisample = 0;
            for(long f = lastFrameIndex; f < frameIndex; f+=stepSize) {
               int sampleIndex = f * numChannels + c;
               float subpixelSample = buffer[sampleIndex]/32565.0;
               if(subpixelSample < losample) {
                  losample = subpixelSample;
               }
               if(subpixelSample > hisample) {
                  hisample = subpixelSample;
               }
            }

            if(losample == 0 && hisample == 0){
               vertex->x = i;
               vertex->y = trackCenter;
               vertex++;
            }
            else {
               if(losample != 0){
                  vertex->x = i;
                  vertex->y = trackCenter - losample * trackHeight*.5;
                  vertex++;
               }
               if(hisample != 0){
                  vertex->x = i;
                  vertex->y = trackCenter - hisample * trackHeight*.5;
                  vertex++;
               }
            }

            while (vertex < & preview.getVertices()[ i * 2 ] + 2) {
               *vertex = *(vertex-1);
               vertex++;
            }

            lastFrameIndex = frameIndex;
         }
         else{
            *vertex++ = ofPoint(i,trackCenter);
            *vertex++ = ofPoint(i,trackCenter);
         }
      }
      preview.simplify();
      previews.push_back(preview);
   }
   //computedZoomBounds = zoomBounds;
   shouldRecomputePreview = false;
}

ofxTLAudioClipTrack::ofxTLAudioClipTrack(){
   xmlFileName = "_AudioClipTrack.xml";
}

ofxTLAudioClipTrack::~ofxTLAudioClipTrack(){
	
}

void ofxTLAudioClipTrack::drawClip( ofxTLClip* clip ){
   ofxTLClipTrack::drawClip( clip );
   ofxTLAudioClip* audioClip = (ofxTLAudioClip*)clip;

   ofLongRange screenRange( screenXToMillis( bounds.x ), screenXToMillis( bounds.x + bounds.width ) );
   if( audioClip->fileLoaded ){
      ofSetColor(ofColor( 50, 50, 50, 50 ));
      if(audioClip->shouldRecomputePreview || viewIsDirty){
         audioClip->recomputePreview( audioClip -> timeRange - screenRange );
      }

      for(int j = 0; j < audioClip->previews.size(); j++){
         ofPushMatrix();
         ofTranslate( clip->displayRect.getLeft(), bounds.y, 0);
         ofScale( 1, bounds.height/100, 1 );
         audioClip->previews[j].draw();
         ofPopMatrix();
      }
   }
}

string ofxTLAudioClipTrack::getTrackType(){
	return "AudioClipTrack";
}

ofxTLClip* ofxTLAudioClipTrack::newClip(){
   return new ofxTLAudioClip();
}

