import QtQuick 2.2
import QtCharts 2.4
import Qt.labs.folderlistmodel 2.11


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
	Rectangle
	{
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
		anchors.right: parent.right
		color: pal.highlightedText
		onTextChanged: fmodel.folder = path.text
	}
	ChartView
	{
		id: dirchart 
		anchors.top: path.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		backgroundColor: pal.window
		legend.alignment: Qt.AlignRight
		legend.labelColor: pal.text
		title: "Top 10 files you want to remove"
		titleColor: pal.text
		titleFont.pointSize: 15
		titleFont.bold: true 
		//theme: ChartView.ChartThemeDark
		PieSeries { id: pieser }

		FolderListModel
		{
			id: fmodel
			showDirs: false
			showDotAndDotDot: false
			sortField: "Size"
			nameFilters: ["*"]

			onStatusChanged:
			{
				if (fmodel.status == FolderListModel.Ready)
				{
					pieser.clear()
					var count = (fmodel.count < 10) ? fmodel.count : 10

					for (var i =0; i < count; i++)
						pieser.append(fmodel.get(i, "fileName"), fmodel.get(i, "fileSize"))

					if (fmodel.count > 10)
					{
						var size = 0

						for (var i = 11; i < fmodel.count; i++)
							size += fmodel.get(i, "fileSize")

						pieser.append("... (" + Number(fmodel.count - 10) + " file(s))", size)
					}
				}
			}
		}
	}
	function myListLoad(FileToLoad, ShowFlags)
	{
		path.text = FileToLoad
		return true
	}
	function myListLoadNext(FileToLoad, ShowFlags)
	{
		return myListLoad(FileToLoad, ShowFlags)
	}
	function myListSendCommand(Command, Parameter)
	{
		return false
	}
	function myListSearchDialog(FindNext)
	{
		return true
	}
	function myListSearchText(SearchString, SearchParameter)
	{
		return false
	}
	function myListPrint(FileToPrint, DefPrinter, PrintFlags, Margins)
	{
		return false
	}
	function myListCloseWindow() 
	{
		return
	}
}
