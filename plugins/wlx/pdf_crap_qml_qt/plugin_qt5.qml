import QtQuick 2.15
import QtQuick.Controls 2.14
import QtQuick.Shapes 1.3
import QtQuick.Pdf 5.15

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
	PdfMultiPageView
	{
		id: pdfView
		anchors.fill: parent
		focus: true
	}
	PdfDocument
	{
		id: doc
	}
	PdfSelection
	{
    		id: selection
    		document: doc
    		fromPoint: textSelectionDrag.centroid.pressPosition
    		toPoint: textSelectionDrag.centroid.position
    		hold: !textSelectionDrag.active
	}
	Shape
	{
    		ShapePath
		{
       			PathMultiline { paths: selection.geometry }
    		}
	}
	DragHandler
	{
    		id: textSelectionDrag
    		acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
    		target: null
	}
	MouseArea
	{
		anchors.fill: pdfView
		onWheel:
		{
        		if (wheel.modifiers & Qt.ControlModifier)
				pdfView.renderScale += wheel.angleDelta.y / 1200
			else
				wheel.accepted = false
		}
	}
	function myListLoad(FileToLoad: string, ShowFlags: int): bool
	{
		doc.source = encodeURIComponent(FileToLoad)

		if (doc.status == PdfDocument.Error)
			return false

		pdfView.document = doc

		return true
	}
	function myListLoadNext(FileToLoad: string, ShowFlags: int): bool
	{
		return myListLoad(FileToLoad, ShowFlags)
	}
	function myListSendCommand(Command: int, Parameter: int): bool
	{
		switch (Command)
		{
		case lc_copy:
			pdfView.copySelectionToClipboard()
			break
		case lc_selectall:
			pdfView.selectAll()
			break
		}

		return true
	}
	function myListSearchText(SearchString: string, SearchParameter: int): bool
	{
		pdfView.searchString = SearchString

		if (SearchParameter & lcs_backwards)
			pdfView.searchBack()
		else
			pdfView.searchForward()

		return true
	}
	onWidthChanged:
	{
 		// pdfView.scaleToWidth(root.width, root.height)
	}


	function myListSearchDialog(FindNext: bool): bool
	{
		return false
	}
	function myListPrint(FileToPrint: string, DefPrinter: string, PrintFlags: int, Margins: rect): bool
	{
		return false
	}
	function myListCloseWindow()
	{
		return false
	}
}
