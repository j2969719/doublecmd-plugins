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
	onWidthChanged:
	{
		if (img.sourceSize.width > root.width / 3 || img.sourceSize.height > root.height / 3)
			img.fillMode = Image.PreserveAspectFit
		else
			img.fillMode = Image.Pad
	}
	onHeightChanged:
	{
		if (img.sourceSize.width > root.width / 3 || img.sourceSize.height > root.height / 3)
			img.fillMode = Image.PreserveAspectFit
		else
			img.fillMode = Image.Pad
	}
	Rectangle
	{
		id: textbg
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: path.bottom
		color: pal.highlight
	}
	Text
	{
		id: path
		anchors.top: parent.top
		anchors.left: parent.left
		color: pal.highlightedText
	}
	Text
	{
		id: sizeinfo
		anchors.top: parent.top
		anchors.left: path.right
		anchors.right: parent.right
		color: pal.highlightedText
	}
	AnimatedImage
	{
		id: img
		anchors.top: path.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		onStatusChanged: playing = (status == AnimatedImage.Ready)
		onSourceSizeChanged:
		{
			if (img.sourceSize.width > root.width / 3 || img.sourceSize.height > root.height / 3)
				img.fillMode = Image.PreserveAspectFit
			else
				img.fillMode = Image.Pad

			sizeinfo.text = " [" + img.sourceSize.width + "x" + img.sourceSize.height + "]"
		}
		MouseArea
		{
			anchors.fill: parent
			onWheel:
			{
				if (wheel.modifiers & Qt.ControlModifier)
					img.scale += img.scale * wheel.angleDelta.y / 120 / 10
			}
		}
	}
	function myListLoad(FileToLoad: string, ShowFlags: int): bool
	{
		// https://ghisler.github.io/WLX-SDK/listload.htm

		/*
		console.log("myListLoad, FileToLoad:", FileToLoad, ", ShowFlags:", ShowFlags)
		console.log("Default INI: ", default_cfg)
		console.log("Plugin Directory: ", plugin_dir)

		if (ShowFlags & lcp_wraptext)
			console.log ("lcp_wraptext")
		if (ShowFlags & lcp_fittowindow)
			console.log ("lcp_fittowindow")
		if (ShowFlags & lcp_fitlargeronly)
			console.log ("lcp_fitlargeronly")
		if (ShowFlags & lcp_center)
			console.log ("lcp_center")
		if (ShowFlags & lcp_ansi)
			console.log ("lcp_ansi")
		if (ShowFlags & lcp_ascii)
			console.log ("lcp_ascii")
		if (ShowFlags & lcp_variable)
			console.log ("lcp_variable")
		if (ShowFlags & lcp_forceshow)
			console.log ("lcp_forceshow")
		*/

		img.source = FileToLoad
		path.text = FileToLoad
		if (!quickview)
		{
			textbg.color = pal.window
			path.color = pal.text
			sizeinfo.color = pal.text
		}
		if (img.status == Image.Error)
			return false
		return true
	}
	function myListLoadNext(FileToLoad: string, ShowFlags: int): bool
	{
		// https://ghisler.github.io/WLX-SDK/listloadnext.htm

		//console.log("myListLoadNext, FileToLoad:", FileToLoad, ", ShowFlags:", ShowFlags)

		return myListLoad(FileToLoad, ShowFlags)
	}
	function myListSendCommand(Command: int, Parameter: int): bool
	{
		// https://ghisler.github.io/WLX-SDK/listsendcommand.htm

		/*
		console.log("myListSendCommand Command:", Command, ", Parameter:", Parameter)
		switch (Command)
		{
		case lc_copy:
			console.log ("lc_copy")
			break
		case lc_selectall:
			console.log ("lc_selectall")
			break
		case lc_setpercent:
			console.log ("lc_setpercent")
			break
		case lc_newparams:
			if (Parameter & lcp_wraptext)
				console.log ("lcp_wraptext")
			if (Parameter & lcp_fittowindow)
				console.log ("lcp_fittowindow")
			if (Parameter & lcp_ansi)
				console.log ("lcp_ansi")
			if (Parameter & lcp_ascii)
				console.log ("lcp_ascii")
			if (Parameter & lcp_variable)
				console.log ("lcp_variable")
			if (Parameter & lcp_fitlargeronly)
				console.log ("lcp_fitlargeronly")
			break
		}
		*/

		return true
	}
	function myListSearchDialog(FindNext: bool): bool
	{
		// https://ghisler.github.io/WLX-SDK/listsearchdialog.htm

		//console.log("myListSearchDialog, FindNext:", FindNext)

		return false
	}
	function myListSearchText(SearchString: string, SearchParameter: int): bool
	{
		// https://ghisler.github.io/WLX-SDK/listsearchtext.htm

		/*
		console.log("myListSearchDialog, SearchString:", SearchString, ", SearchParameter:", SearchParameter)

		if (SearchParameter & lcs_findfirst)
			console.log ("lcs_findfirst")
		if (SearchParameter & lcs_matchcase)
			console.log ("lcs_matchcase")
		if (SearchParameter & lcs_wholewords)
			console.log ("lcs_wholewords")
		if (SearchParameter & lcs_backwards)
			console.log ("lcs_backwards")
		*/

		return false
	}
	function myListPrint(FileToPrint: string, DefPrinter: string, PrintFlags: int, Margins: rect): bool
	{
		// https://ghisler.github.io/WLX-SDK/listprint.htm

		//console.log("myListPrint, FileToPrint:", FileToPrint, ", DefPrinter:", DefPrinter, ", PrintFlags:", PrintFlags, ", Margins:", Margins)

		return false
	}
	function myListCloseWindow()
	{
		// https://ghisler.github.io/WLX-SDK/listclosewindow.htm

		//console.log("myListCloseWindow")
	}
}
