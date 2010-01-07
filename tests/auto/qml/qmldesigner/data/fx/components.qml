import Qt 4.6

Rectangle {
	width: 100
	height: 100
	

	Component {
		id: componentOne
		Rectangle { width: 80; height: 80; color: "lightsteelblue" }
	}
	
	Component {
		id: componentTwo
		Rectangle { width: 80; height: 80; color: "lightsteelblue" }
	}
	
	Loader {
		x: 40
		y: 40
		width: 20
		height: 20	
	    sourceComponent: componentTwo
	}
 
 }