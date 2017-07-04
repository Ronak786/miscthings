function positionMessage() {
	if (!document.getElementById) return false;
	if (!document.getElementById("message")) return false;
	var elem = document.getElementById("message");
	elem.style.position = "absolute";
	elem.style.left = document.documentElement.clientLeft;
	elem.style.top = document.documentElement.clientTop;
//	elem.style.fontSize = "1.5em";
	elem.style.color = "red";
	width = parseInt(document.documentElement.clientWidth);
	height = parseInt(document.documentElement.clientHeight);
	xdir = 1;
	ydir = 1;
//	moveElement("message", 400, 200, 1);
	movement = setInterval("moveMessage()",1);
}

function moveMessage() {
	if (!document.getElementById) return false;
	if (!document.getElementById("message")) return false;
	var elem = document.getElementById("message");
	var xpos = parseInt(elem.style.left);
	var ypos = parseInt(elem.style.top);
	/*
	if (xpos == 200 && ypos == 100) return true;
	if (xpos < 200) xpos++;
	if (xpos > 200) xpos--;
	if (ypos < 100) ypos++;
	if (ypos > 100) ypos--;
	*/
	rate=300;
	var xdis = 0;
	var ydis = 0;
	if (xdir > 0) {
		xdis = Math.ceil((width - xpos)/rate);
	}
	if (ydir > 0) {
		ydis = Math.ceil((height - ypos)/rate);
	}
	if (xdir < 0) {
		xdis = Math.ceil((xpos - 0)/rate);
	}
	if (ydir < 0) {
		ydis = Math.ceil((ypos - 0)/rate);
	}
//	xpos += xdir;
//	ypos += ydir;
	xpos += xdis * xdir;
	ypos += ydis * ydir;
	if (xpos >= width || xpos <= 0) xdir *= -1;
	if (ypos >= height || ypos <= 0) ydir *= -1;
	elem.style.left = xpos + "px";
	elem.style.top = ypos +"px";
}

function moveElement(elementID, final_x, final_y, interval) {
	if (!document.getElementById) return false;
	if (!document.getElementById(elementID)) return false;
	var elem = document.getElementById(elementID);
	if (elem.movement) {
		clearTimeout(elem.movement);
	}
	if (!elem.style.left) {
		elem.style.left = "0px";
	}
	if (!elem.style.top) {
		elem.style.top = "0px";
	}
	var xpos = parseInt(elem.style.left);
	var ypos = parseInt(elem.style.top);
	var dist = 0;

	if (xpos == final_x && ypos == final_y) return true;

	rate=10;
	if (xpos < final_x)  {
		dist = Math.ceil((final_x - xpos)/rate);
		xpos += dist;
	}
	if (ypos < final_y)  {
		dist = Math.ceil((final_y - ypos)/rate);
		ypos += dist;
	}
	if (xpos > final_x)  {
		dist = Math.ceil((xpos - final_x)/rate);
		xpos -= dist;
	}
	if (ypos > final_y)  {
		dist = Math.ceil((ypos - final_y)/rate);
		ypos -= dist;
	}
	elem.style.left = xpos + "px";
	elem.style.top = ypos + "px";
	repeat = "moveElement('" + elementID +"',"+final_x + "," + final_y +","+interval+")"
	elem.movement = setTimeout(repeat);
}

addLoadEvent(positionMessage);
