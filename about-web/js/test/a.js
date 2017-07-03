function heheda() {
	var res = document.getElementsByTagName("h1")[0]
	alert(res.getAttribute("title"))
	res.setAttribute("title", "heheda")
	alert(res.getAttribute("title"))
}

heheda()

