function addLoadEvent(func) {
    var oldonload = window.onload;
    if (typeof window.onload != 'function') {
        window.onload = func;
    } else {
        window.onload = function() {
            oldonload();
            func();
        }
    }   
}

function getNextElement(node) {
	if (node.nodeType == 1) return node;
	if (node.nextSibling) return getNextElement(node.nextSibling);
	return null;
}

function styleElementSiblings(tag, theclass) {
	if (!document.getElementsByTagName) return false;
	var elems = document.getElementsByTagName(tag);
	var elem;
	for (var i=0; i < elems.length; i++) {
		elem = getNextElement(elems[i].nextSibling);
		addClass(elem, theclass);
	}
}

function addClass(element, value) {
	if (!element.className) {
		element.className=value;
	} else {
		newClassName = element.className;
		newClassName += " ";
		newClassName += value;
		element.className = newClassName;
	}
}
