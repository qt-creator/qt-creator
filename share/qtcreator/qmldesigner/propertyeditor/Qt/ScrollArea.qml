import Qt 4.6
import Bauhaus 1.0


QScrollArea {

 property var finished;

   onFinishedChanged: {
      setupProperWheelBehaviour();
   }


}