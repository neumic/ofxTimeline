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
#include "ofSystemUtils.h"

ofxTLClip::ofxTLClip(){
   selected = false;
   hovering = false;
   movedSinceUpdate = true;
   playerOffset = 0;
   filePath = "";
   fileName = "No File Loaded";
   draggingStart = false;
   draggingEnd = false;
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

void ofxTLClip::setClipPosition( long millis ){
   //Set the position of the player from timeline milliseconds
   setPlayerPosition( millis - timeRange.min - playerOffset );
}

void ofxTLClip::setPlayerPosition( long millis ){
   cerr << "player Position set to: " << millis <<endl;
}

long ofxTLClip::getPlayerDuration( ){
   return 10000;
}

void ofxTLClip::clampedMove( long millisOffset, long lower, long upper ){
   //Moves clip, but not past boundaries
   millisOffset = min( millisOffset, upper - timeRange.max );
   millisOffset = max( millisOffset, lower - timeRange.min );
   timeRange += millisOffset;
   if( millisOffset != 0 ){
      movedSinceUpdate = true;
   }
}

void ofxTLClip::clampedGrabMove( long millisOffset, long lower, long upper ){
   //Moves clip begining to millis, but not past boundaries
   millisOffset = (grabTime + millisOffset) - timeRange.min;
   clampedMove( millisOffset, lower, upper );
}

void ofxTLClip::clampedDragStart( long millisOffset ){
   if( grabPlayerOffset - millisOffset > 0){
      millisOffset = grabPlayerOffset;
   }
   if( grabTime + millisOffset > timeRange.max){
      millisOffset = timeRange.max - grabTime;
   }
   timeRange.min = millisOffset + grabTime;
   playerOffset  = grabPlayerOffset - millisOffset;
}

void ofxTLClip::clampedDragEnd( long millisOffset ){
   if( grabTime + millisOffset > timeRange.min + playerOffset + getPlayerDuration() ){
      millisOffset = timeRange.min + playerOffset + getPlayerDuration() - grabTime;
   }
   if( millisOffset < timeRange.min - grabTime ){
      millisOffset = timeRange.min - grabTime;
   }
   timeRange.max = grabTime + millisOffset;
}

bool ofxTLClip::loadFile( string path ){
   filePath = path;
   fileName = ofFilePath::getFileName( filePath );
   return true;
}

string ofxTLClip::getFilePath(){
   return filePath;
}

string ofxTLClip::getFileName(){
   return fileName;
}

void ofxTLClip::beginHover(){
   hovering = true;
}

void ofxTLClip::endHover(){
   hovering = false;
}

bool ofxTLClip::isHovering(){
   return hovering;
}

void ofxTLClip::storeXml( ofxXmlSettings* savedClips ){
   //Doesn't write outer "clip" tags so subclasses can easily extend
   savedClips -> addValue("timeBegin", 
         ofxTimecode::timecodeForMillis(timeRange.min));
   savedClips -> addValue("timeEnd",
         ofxTimecode::timecodeForMillis(timeRange.max));
   savedClips -> addValue("filepath", getFilePath());
}

ofxTLClipTrack::ofxTLClipTrack(){
   createNewPoint = false;
   isDraggingClips = false;
   clickedInModalBox = false;
   drawingModalBox = false;
   xmlFileName = "_ClipTrack.xml";
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
      if( !timeline->getInOutRangeMillis().intersects(clips[i] -> timeRange) ){
         //ignore clips outside of the playing range
         continue;
      }
      if( ( clips[i] -> timeRange.contains( lastTimelinePoint ) ||
            clips[i] -> movedSinceUpdate ) &&
          !clips[i] -> timeRange.contains( thisTimelinePoint ) ) {
          clips[i] -> stop();
      }
      else if( clips[i] -> timeRange.contains( thisTimelinePoint ) &&
               (!clips[i] -> timeRange.contains( lastTimelinePoint ) ||
                 clips[i] -> movedSinceUpdate ) ){
         if( timeline->getIsPlaying() ){
            clips[i] -> setClipPosition( thisTimelinePoint );
            clips[i] -> play();
         }
      }
      clips[i] -> movedSinceUpdate = false;
   }
   lastTimelinePoint = thisTimelinePoint;
}

//draw your track contents. use ofRectangle bounds to know where to draw
//and the Track functions screenXToMillis() or millisToScreenX() to respect zoom
void ofxTLClipTrack::draw(){
	ofPushStyle();
	ofNoFill();
	for(int i = 0; i < clips.size(); i++){
      drawClip( clips[i] );
	}
}

void ofxTLClipTrack::drawClip( ofxTLClip* clip ){
   clip -> displayRect = ofRectangle( millisToScreenX(clip -> timeRange.min),
           bounds.getMinY(),
           millisToScreenX(clip -> timeRange.max) - 
              millisToScreenX(clip -> timeRange.min),
           bounds.height );
   if(clip->displayRect.getRight() > bounds.x && 
      clip->displayRect.getLeft() < bounds.x+bounds.width){
      if( clip -> isSelected() ){
         ofFill;
         ofSetColor(timeline->getColors().textColor);
      } else {
         ofSetColor(timeline->getColors().keyColor);
      }
      ofRect( clip -> displayRect );
      //Draw handles
      if( clip -> isHovering() && !ofGetModifierSelection() ){
         ofTriangle( clip->displayRect.getLeft() + 10, bounds.y,
                     clip->displayRect.getLeft() + 10, bounds.y,
                     clip->displayRect.getLeft(),      bounds.y + 10 );
         ofTriangle( clip->displayRect.getRight() - 10, bounds.y + bounds.height,
                     clip->displayRect.getRight() - 10, bounds.y + bounds.height,
                     clip->displayRect.getRight(),      bounds.y + bounds.height - 10 );
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

      OFX_TIMELINE_FONT_RENDERER font = timeline -> getFont();
      modalBox = ofRectangle( millisToScreenX( selectedClip->timeRange.min ),
                              bounds.y+bounds.height,
                              font.stringWidth( selectedClip->getFileName() ) + 10, 20);
      ofRect( modalBox );
      ofSetColor(20, 20, 20);
      timeline->getFont().drawString( selectedClip->getFileName(),
         modalBox.x + 5, modalBox.y + 15);
      ofPopStyle();
   }
}

void ofxTLClipTrack::playbackStarted(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackStarted(args);
   for(int i = 0; i < clips.size(); i++){
      if( clips[i] -> timeRange.contains( currentTrackTime() ) ){
         clips[i] -> setClipPosition( currentTrackTime() );
         clips[i] -> play();
      }
   }
}

void ofxTLClipTrack::playbackEnded(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackEnded(args);
   for(int i = 0; i < clips.size(); i++){
      clips[i] -> stop();
   }
}

void ofxTLClipTrack::playbackLooped(ofxTLPlaybackEventArgs& args){
   ofxTLTrack::playbackLooped(args);
}

//caled by the timeline, don't need to register events
bool ofxTLClipTrack::mousePressed(ofMouseEventArgs& args, long millis){
	if(drawingModalBox){
		clickedInModalBox = args.button == 0 && modalBox.inside(args.x, args.y);
		if(clickedInModalBox){
         //do button stuffs
		}
		return true;
	}

   for( int i = 0; i < clips.size(); i++ ){
      clips[i] -> grabTime = clips[i] -> timeRange.min;
      clips[i] -> grabPlayerOffset = clips[i] -> playerOffset;
   }
   grabTime = millis;
	createNewPoint = isActive();
   isDraggingClips = !ofGetModifierSelection(); //only drag if clicked without shift

   if( !isActive() ){
      return false;
   }

   bool shouldUnselectAll = true;

   for( int i = 0; i < clips.size(); i++ ){
      if( clips[i] -> isInside( millis ) ){
         shouldUnselectAll = false;
         createNewPoint = false;
         selectedClip = clips[i];
         if( !ofGetModifierSelection() ){
            //If the click is within this clip and shift isn't down
            if( !clips[i] -> isSelected() ){
               timeline->unselectAll();
               clips[i] -> select();
            }
            if( (args.x - clips[i] -> displayRect.getLeft() ) + 
                (args.y - clips[i] -> displayRect.getTop() ) <= 10){
               timeline->unselectAll();
               clips[i] -> select();
               clips[i] -> draggingStart = true;
            }
            else if( (clips[i] -> displayRect.getRight() - args.x) + 
                     (clips[i] -> displayRect.getBottom() - args.y) <= 10){
               timeline->unselectAll();
               clips[i] -> select();
               clips[i] -> draggingEnd = true;
               //I'd like to find a better way to get this to clampedDragEnd
               clips[i] -> grabTime = clips[i] -> timeRange.max;
            }
         }
         else {
            //If the click is within this clip and shift is down
            if(clips[i] -> isSelected() ){
               clips[i] -> deselect();
               selectedClip = NULL;
            }
            else {
               clips[i] -> select();
            }
         } 
      }
   }
   if( shouldUnselectAll ){
      timeline->unselectAll();
   }

   if( selectedClip != NULL ){
      //if we're in the first or last 20 secs of clip, snap to the closer edge,
      //otherwise don't snap at all.
      //TODO: This isn't very good, since there's no indication of when it's
      //going to snap or not. Also, 20 sec is a variable number of pixels
      if( grabTime <= selectedClip->timeRange.center() &&
          grabTime <= selectedClip->timeRange.min + 20000){
         timeline->setDragTimeOffset(grabTime - selectedClip->timeRange.min);
      }
      else if( grabTime > selectedClip->timeRange.center() &&
               grabTime > selectedClip->timeRange.max - 20000){
         timeline->setDragTimeOffset(grabTime - selectedClip->timeRange.max);
      }
      else{
         timeline->cancelSnapping(); 
      }
   }

	clickPoint = ofVec2f(args.x,args.y);
	return createNewPoint; //signals that the click made a selection
}

void ofxTLClipTrack::mouseMoved(ofMouseEventArgs& args, long millis){
   for( int i = 0; i < clips.size(); i++ ){
      if( clips[i] -> displayRect.inside( args.x, args.y ) ){
         clips[i] -> beginHover();
      }
      else{
         clips[i] -> endHover();
      }
   }
}
void ofxTLClipTrack::mouseDragged(ofMouseEventArgs& args, long millis){
   if( isDraggingClips ){
   //if click was initiated without shift pressed
      for( int i = 0; i < clips.size(); i++ ){
         if( clips[i] -> draggingStart ){
            clips[i] -> clampedDragStart( millis - grabTime );
         }
         else if( clips[i] -> draggingEnd ){
            clips[i] -> clampedDragEnd( millis - grabTime );
         }
         else if( clips[i] -> isSelected() ){
            clips[i] -> clampedGrabMove( millis - grabTime, 0, 
                                     timeline -> getDurationInMilliseconds() );
         }
      }
   }
}
void ofxTLClipTrack::mouseReleased(ofMouseEventArgs& args, long millis){
   if( drawingModalBox ){
      if( !clickedInModalBox ){
         timeline->dismissedModalContent();
         drawingModalBox = false;
         return;
      }
      else{
         if( selectedClip->loadFile( ofSystemLoadDialog( "Load Clip" ).getPath() ) ){
            timeline->dismissedModalContent();
            drawingModalBox = false;
         }
      }
         
   }

	grabTime = 0;
   isDraggingClips = false;
	//need to create clicks on mouse up if the mouse hasn't moved in order to work
	//well with the click-drag rectangle thing
	if(args.button == 2 && clickPoint.distance(ofVec2f(args.x, args.y)) < 4){
      if( createNewPoint ){
         selectedClip = newClip();
         //newpoint.value = ofMap(args.y, bounds.getMinY(), bounds.getMaxY(), 0, 1.0);
         selectedClip -> select();
         selectedClip -> timeRange = ofLongRange(millis, millis + 10000);
         clips.push_back(selectedClip);
         drawingModalBox = true;
         timeline->presentedModalContent(this);
         //call this on mouseup or keypressed after a click 
         //will trigger save and needed for undo
         timeline->flagTrackModified(this);
      }
      else{
         for( int i = 0; i < clips.size(); i++ ){
            if( clips[i] -> isSelected() && clips[i] -> timeRange.contains( millis ) ){
               selectedClip = clips[i];
               drawingModalBox = true;
               timeline->presentedModalContent(this);
            }
         }
      }
	}
   for( int i = 0; i < clips.size(); i++ ){
      clips[i] -> draggingStart = false;
      clips[i] -> draggingEnd = false;
   }
}

bool clipIsSelected( ofxTLClip clip ){return clip.isSelected();}

//keys pressed events, and nuding from arrow keys with normalized nudge amount 0 - 1.0
void ofxTLClipTrack::keyPressed(ofKeyEventArgs& args){
	if(args.key == OF_KEY_DEL || args.key == OF_KEY_BACKSPACE){
      //TODO: Fix this crude hacky delete
      ofxTLClip* deleteMe = new ofxTLClip;
      for( int i = 0; i < clips.size(); i++ ){
         if( clips[i] -> isSelected() ){
            delete clips[i];
            clips[i] = deleteMe;
         }
      }
		clips.erase( remove( clips.begin(), clips.end(), deleteMe), clips.end() );
      delete deleteMe;
	}
	if(drawingModalBox){
		if(args.key == OF_KEY_RETURN){
			timeline->dismissedModalContent();
			drawingModalBox = false;
		}
	}
}
void ofxTLClipTrack::nudgeBy(ofVec2f nudgePercent){
   for( int i = 0; i < clips.size(); i++ ){
      if( clips[i] -> isSelected() ){
         clips[i] -> clampedMove(
            timeline->getDurationInMilliseconds() * nudgePercent.x,
            0, timeline->getDurationInMilliseconds()
         );
      }
   }
}

//if your track has some selectable elements you can interface with snapping
//and selection/unselection here
void ofxTLClipTrack::getSnappingPoints(set<unsigned long long>& points){
   for( int i = 0; i < clips.size(); i++){
      //TODO check if the clip is in bounds as well
      if( !clips[i] -> isSelected() ){
         points.insert(clips[i] -> timeRange.min);
         points.insert(clips[i] -> timeRange.max);
      }
   }
}

void ofxTLClipTrack::regionSelected(ofLongRange timeRange, ofRange valueRange){
   for( int i = 0; i < clips.size(); i++ ){
      if( timeRange.contains( clips[i] -> timeRange ) ){
         clips[i] -> select();
      } else if( !ofGetModifierSelection() ){
         clips[i] -> deselect();
      }
   }
}

void ofxTLClipTrack::unselectAll(){
   for( int i = 0; i < clips.size(); i++ ){
      clips[i] -> deselect();
   }
}

void ofxTLClipTrack::selectAll(){
   for( int i = 0; i < clips.size(); i++ ){
      clips[i] -> select();
   }
}

int ofxTLClipTrack::getSelectedItemCount(){
   int count = 0;
   for( int i = 0; i < clips.size(); i++ ){
      if(clips[i] -> isSelected()){
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
	ofxXmlSettings savedClips;
	savedClips.addTag("clipTrack");
	savedClips.pushTag("clipTrack");

	for(int i = 0; i < clips.size(); i++){
      savedClips.addTag("clip");
      savedClips.pushTag("clip", i);
      clips[i] -> storeXml(&savedClips);
      savedClips.popTag(); //"clip"
	}

	savedClips.popTag();//"clipTrack"
	string str;
	savedClips.copyXmlToString(str);
	return str;
}

void ofxTLClipTrack::loadFromXMLRepresentation(string rep){

}

//serialize your track.
//use ofxTLTrack's string xmlFileName
void ofxTLClipTrack::save(){
   string xmlRep = getXMLRepresentation();
   ofxXmlSettings savedkeyframes;
   savedkeyframes.loadFromBuffer(xmlRep);
   savedkeyframes.saveFile(xmlFileName);
	
}

void ofxTLClipTrack::load(){
   ofxXmlSettings savedClips;
   if(!savedClips.loadFile(xmlFileName)){
      ofLog(OF_LOG_NOTICE, "ofxTLClipTrack --- couldn't load xml file " + xmlFileName);
      return;
   }

   savedClips.pushTag("clipTrack");
   int numKeyTags = savedClips.getNumTags("clip");
   for(int i = 0; i < numKeyTags; i++){
      savedClips.pushTag("clip", i);
      ofxTLClip* clip = newClip();
      
      string timecode = savedClips.getValue("timeBegin", "00:00:00:000");
      clip -> timeRange.min = timeline->getTimecode().millisForTimecode(timecode);
      timecode = savedClips.getValue("timeEnd", "00:00:00:000");
      clip -> timeRange.max = timeline->getTimecode().millisForTimecode(timecode);
      clip -> loadFile( savedClips.getValue("filepath", "") );
      savedClips.popTag(); //clip
      clips.push_back( clip );
   }
   
   savedClips.popTag(); //clipTrack

   timeline->flagTrackModified(this);
	
}

void ofxTLClipTrack::clear(){
	
}

ofxTLClip* ofxTLClipTrack::newClip(){
   //To be overridden
   return new ofxTLClip();
}
