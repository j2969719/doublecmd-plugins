import QtQuick 2.2

Rectangle
{
	id: root
	SystemPalette
	{
		id: pal
		colorGroup: SystemPalette.Active
	}
	anchors.centerIn: parent
	color: pal.window
	AnimatedImage
	{
		id: img
		anchors.fill: parent
		fillMode: Image.PreserveAspectFit
	}
	function myListLoad(FileToLoad, ShowFlags)
	{
		// console.log("myListLoad, FileToLoad:", FileToLoad, ", ShowFlags:", ShowFlags)
		img.source = FileToLoad
		if (img.status == Image.Error)
			return false
		return true
	}
	function myListSendCommand(Command, Parameter)
	{
		// https://ghisler.github.io/WLX-SDK/listplug.h.htm
		// console.log("myListSendCommand Command:", Command, ", Parameter:", Parameter)
	}
	function myListSearchDialog(FindNext)
	{
		// console.log("myListSearchDialog, FindNext:", FindNext)
	}
	function myListCloseWindow()
	{
		// console.log("myListCloseWindow")
	}
}