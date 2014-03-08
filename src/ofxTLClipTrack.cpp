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

#include "ofxTLClipTrack.h"
#include "ofxTimeline.h"
#include "ofxHotKeys.h"

ofxTLClip::ofxTLClip(){
   selected = false;
}

bool ofxTLClip::isInside( long millis ){
   return timeRange.contains( millis );
}

bool ofxTLClip::isSelected(){
   return selected;
}

void ofxTLClip::select(){
   selected = true;
}

void ofxTLClip::deselect(){
   selected = false;
}

void ofxTLClip::play(){
   cerr << "Clip Playing" << endl;
}

void ofxTLClip::stop(){
   cerr << "Clip Stopped" << endl;
}

void ofxTLClip::setPosition( long millis ){
   cerr << "Clip Position set to: " << millis - timeRange.min <<endl;
}

ofxTLClipTrack::ofxTLClipTrack(){
	
}

ofxTLClipTrack::~ofxTLClipTrack(){
	
}

//enable and disable are always automatically called
//in setup. Must call superclass's method as well as doing your own
//enabling and disabling
void ofxTLClipTrack::enable(){
	ofxTLTrack::enable();
	
	//other enabling
}

void ofxTLClipTrack::disable(){
	ofxTLTrack::disable();
	
	//other disabling
}

//update is called every frame.
//if your track triggers events it's good to do it here
//if timeline is set to thread this is called on the back thread so
//be careful if loading images in herre
void ofxTLClipTrack::update(){
   long thisTimelinePoint = currentTrackTime();
   for(int i = 0; i < clips.size(); i++){
      if( !timeline->getInOutRangeMillis().intersects(clips[i].timeRange) ){
         //ignore clips outside of the playing range
         continue;
      }
      if( clips[i].timeRange.contains( lastTimelinePoint ) &&
          !clips[i].timeRange.contains( thisTimelinePoint ) ) {
          clips[i].stop();
      }
      else if( clips[i].timeRange.contains( thisTimelinePoint ) &&
               !clips[i].timeRange.contains( lastTimelinePoint ) ){
         if( timeline->getIsPlaying() ){
            clips[i].setPosition( thisTimelinePoint );
            clips[i].play();
         }
      }
   }
   lastTimelinePoint = thisTimelinePoint;
}

//draw your track contents. use ofRectangle bounds to know where to draw
//and the Track functions screenXToMillis() or millisToScreenX() to respect zoom
void ofxTLClipTrack::draw(){
	
	//this is just a simple example
	ofPushStyle();
	ofNoFill();
	for(int i = 0; i < clips.size(); i++){
		float boxStart = millisToScreenX(clips[i].timeRange.min);
		float boxWidth = millisToScreenX(clips[i].timeRange.max) - millisToScreenX(clips[i].timeRange.min);
		if(boxStart + boxWidth > bounds.x && boxStart < bounds.x+bounds.width){
			//float screenY = ofMap(clickPoints[i].value, 0.0, 1.0, bounds.getMinY(), bounds.getMaxY());
			//ofCircle(screenX, bounds.getMinY() + 10, 4);
         if( clips[i].isSelected() ){
            ofSetColor(timeline->getColors().textColor);
         } else {
            ofSetColor(timeline->getColors().keyColor);
         }
         ofRect(boxStart, bounds.getMinY(), boxWidth, bounds.height );
		}
	}
}

void ofxTLClipTrack::drawModalContent(){
   if(drawingModalBox){
      if( selectedClip == NULL ){
         drawingModalBox = false;
         timeline->dismissedModalContent();
         return;
      }
      ofPushStyle();
      ofFill();
      ofSetColor(255);

      modalBox = ofRectangle( millisToScreenX( selectedClip->timeRange.min ), bounds.y+bounds.height, 200, 200);
      ofRect( modalBox );
      ofPopStyle();
   }
}

void ofxTLClipTrack::playbackStarted(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackStarted(args);
   for(int i = 0; i < clips.size(); i++){
      if( clips[i].timeRange.contains( currentTrackTime() ) ){
         clips[i].setPosition( currentTrackTime() );
         clips[i].play();
      }
   }
}

void ofxTLClipTrack::playbackEnded(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackEnded(args);
   for(int i = 0; i < clips.size(); i++){
      clips[i].stop();
   }
}

void ofxTLClipTrack::playbackLooped(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackLooped(args);
}

//caled by the timeline, don't need to register events
bool ofxTLClipTrack::mousePressed(ofMouseEventArgs& args, long millis){
   grabTimeOffset = millis;
	createNewPoint = isActive();
   isDraggingClips = !ofGetModifierSelection(); //only drag if clicked without shift
   if( !isActive() ){
      return false;
   }

   bool shouldUnselectAll = true;

   for( int i = 0; i < clips.size(); i++ ){
      if( clips[i].isInside( millis ) ){
         shouldUnselectAll = false;
         if( !ofGetModifierSelection() ){
            //If the click is within this clip and shift isn't down
            if( !clips[i].isSelected() ){
               timeline->unselectAll();
               clips[i].select();
               createNewPoint = false;
            }
         }
         else {
            //If the click is within this clip and shift is down
            if(clips[i].isSelected() ){
               clips[i].deselect();
            }
            else {
               clips[i].select();
            }
         } 
      }
   }
   if( shouldUnselectAll ){
      timeline->unselectAll();
   }
	clickPoint = ofVec2f(args.x,args.y);
	return createNewPoint; //signals that the click made a selection
}

void ofxTLClipTrack::mouseMoved(ofMouseEventArgs& args, long millis){
	
}
void ofxTLClipTrack::mouseDragged(ofMouseEventArgs& args, long millis){
   if( isDraggingClips ){
   //if click was initiated without shift pressed
      for( int i = 0; i < clips.size(); i++ ){
         if( clips[i].isSelected() ){
            clips[i].timeRange += millis - grabTimeOffset;
         }
      }
      grabTimeOffset = millis;
   }
}
void ofxTLClipTrack::mouseReleased(ofMouseEventArgs& args, long millis){
	grabTimeOffset = 0;
   isDraggingClips = false;
	//need to create clicks on mouse up if the mouse hasn't moved in order to work
	//well with the click-drag rectangle thing
	if(args.button == 2 && createNewPoint && clickPoint.distance(ofVec2f(args.x, args.y)) < 4){
		ofxTLClip newClip;
		//newpoint.value = ofMap(args.y, bounds.getMinY(), bounds.getMaxY(), 0, 1.0);
		newClip.timeRange = ofLongRange(millis, millis + 10000);
		clips.push_back(newClip);
		//call this on mouseup or keypressed after a click 
		//will trigger save and needed for undo
		timeline->flagTrackModified(this);
	}
}

bool clipIsSelected( ofxTLClip clip ){return clip.isSelected();}

//keys pressed events, and nuding from arrow keys with normalized nudge amount 0 - 1.0
void ofxTLClipTrack::keyPressed(ofKeyEventArgs& args){
	if(args.key == OF_KEY_DEL || args.key == OF_KEY_BACKSPACE){
		clips.erase( remove_if( clips.begin(), clips.end(), clipIsSelected), clips.end() );
	}
}
void ofxTLClipTrack::nudgeBy(ofVec2f nudgePercent){
	
}

//if your track has some selectable elements you can interface with snapping
//and selection/unselection here
void ofxTLClipTrack::getSnappingPoints(set<unsigned long long>& points){
   for( int i = 0; i < clips.size(); i++){
      //TODO check if the clip is in bounds as well
      if( !clips[i].isSelected() ){
         points.insert(clips[i].timeRange.min);
         points.insert(clips[i].timeRange.max);
      }
   }
}

void ofxTLClipTrack::regionSelected(ofLongRange timeRange, ofRange valueRange){
   for( int i = 0; i < clips.size(); i++ ){
      if( timeRange.contains( clips[i].timeRange ) ){
         clips[i].select();
      } else if( !ofGetModifierSelection() ){
         clips[i].deselect();
      }
   }
}

void ofxTLClipTrack::unselectAll(){
   for( int i = 0; i < clips.size(); i++ ){
      clips[i].deselect();
   }
}

void ofxTLClipTrack::selectAll(){
   for( int i = 0; i < clips.size(); i++ ){
      clips[i].select();
   }
}

int ofxTLClipTrack::getSelectedItemCount(){
   int count = 0;
   for( int i = 0; i < clips.size(); i++ ){
      if(clips[i].isSelected()){
         count++;
      }
   }
   return count;
}

//return a unique name for your track
string ofxTLClipTrack::getTrackType(){
	return "ClipTrack";
}

//for copy+paste you can optionaly implement ways
//of creating XML strings that represent your selected tracks
string ofxTLClipTrack::copyRequest(){
	return "";
}

string ofxTLClipTrack::cutRequest(){
	return "";
}

//will return the same type of strings you provide in copy and paste
//but may contain foreign data from other tracks so be careful
void ofxTLClipTrack::pasteSent(string pasteboard){
	
}

//for undo and redo you can implement a way of
//reperesnt your whole track as XML
string ofxTLClipTrack::getXMLRepresentation(){
	return "";
}

void ofxTLClipTrack::loadFromXMLRepresentation(string rep){

}

//serialize your track.
//use ofxTLTrack's string xmlFileName
void ofxTLClipTrack::save(){
	
}

void ofxTLClipTrack::load(){
	
}

void ofxTLClipTrack::clear(){
	
}
