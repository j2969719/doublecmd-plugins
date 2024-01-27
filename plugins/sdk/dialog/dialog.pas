unit dialog;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, StdCtrls, ExtCtrls;

type

  { TDialogBox }

  TDialogBox = class(TForm)
    procedure ButtonClick(Sender: TObject);
    procedure ButtonEnter(Sender: TObject);
    procedure ButtonExit(Sender: TObject);
    procedure ButtonKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure ButtonKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure CheckBoxChange(Sender: TObject);
    procedure ComboBoxChange(Sender: TObject);
    procedure ComboBoxClick(Sender: TObject);
    procedure ComboBoxDblClick(Sender: TObject);
    procedure ComboBoxEnter(Sender: TObject);
    procedure ComboBoxExit(Sender: TObject);
    procedure ComboBoxKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState
      );
    procedure ComboBoxKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure DialogBoxClose(Sender: TObject; var CloseAction: TCloseAction);
    procedure DialogBoxShow(Sender: TObject);
    procedure EditChange(Sender: TObject);
    procedure EditClick(Sender: TObject);
    procedure EditDblClick(Sender: TObject);
    procedure EditEnter(Sender: TObject);
    procedure EditExit(Sender: TObject);
    procedure EditKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure EditKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure ListBoxClick(Sender: TObject);
    procedure ListBoxDblClick(Sender: TObject);
    procedure ListBoxEnter(Sender: TObject);
    procedure ListBoxExit(Sender: TObject);
    procedure ListBoxKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState
      );
    procedure ListBoxKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure ListBoxSelectionChange(Sender: TObject; User: boolean);
    procedure TimerTimer(Sender: TObject);
  private

  public

  end;

var
  DialogBox: TDialogBox;

implementation

{$R *.lfm}

{ TDialogBox }

procedure TDialogBox.ButtonClick(Sender: TObject);
begin

end;

procedure TDialogBox.ButtonEnter(Sender: TObject);
begin

end;

procedure TDialogBox.ButtonExit(Sender: TObject);
begin

end;

procedure TDialogBox.ButtonKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.ButtonKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.CheckBoxChange(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxChange(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxClick(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxDblClick(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxEnter(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxExit(Sender: TObject);
begin

end;

procedure TDialogBox.ComboBoxKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.ComboBoxKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.DialogBoxClose(Sender: TObject;
  var CloseAction: TCloseAction);
begin

end;

procedure TDialogBox.DialogBoxShow(Sender: TObject);
begin

end;

procedure TDialogBox.EditChange(Sender: TObject);
begin

end;

procedure TDialogBox.EditClick(Sender: TObject);
begin

end;

procedure TDialogBox.EditDblClick(Sender: TObject);
begin

end;

procedure TDialogBox.EditEnter(Sender: TObject);
begin

end;

procedure TDialogBox.EditExit(Sender: TObject);
begin

end;

procedure TDialogBox.EditKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.EditKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.ListBoxClick(Sender: TObject);
begin

end;

procedure TDialogBox.ListBoxDblClick(Sender: TObject);
begin

end;

procedure TDialogBox.ListBoxEnter(Sender: TObject);
begin

end;

procedure TDialogBox.ListBoxExit(Sender: TObject);
begin

end;

procedure TDialogBox.ListBoxKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.ListBoxKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin

end;

procedure TDialogBox.ListBoxSelectionChange(Sender: TObject; User: boolean);
begin

end;

procedure TDialogBox.TimerTimer(Sender: TObject);
begin

end;

end.

