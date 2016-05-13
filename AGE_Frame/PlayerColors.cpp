#include "../AGE_Frame.h"

uint8_t AGE_SLP::playerColorStart = 0;
uint8_t AGE_SLP::playerColorID = 0;
AGE_SLP::SHOW AGE_SLP::currentDisplay = AGE_SLP::SHOW::NONE;

void AGE_Frame::OnPlayerColorsSearch(wxCommandEvent &event)
{
	How2List = SEARCH;
	ListPlayerColors();
}

string AGE_Frame::GetPlayerColorName(int index)
{
	if(GenieVersion < genie::GV_AoKE3)
		return dataset->PlayerColours[index].Name;
	return "Color "+lexical_cast<string>(index)+" ";
}

void AGE_Frame::ListPlayerColors()
{
	InitPlayerColors();
	wxTimerEvent E;
	OnPlayerColorsTimer(E);
}

void AGE_Frame::InitPlayerColors()
{
	InitSearch(Colors_Colors_Search->GetValue().MakeLower(), Colors_Colors_Search_R->GetValue().MakeLower());

	Colors_Colors_ListV->names.clear();
	Colors_Colors_ListV->indexes.clear();
	wxArrayString names;
	names.Alloc(dataset->PlayerColours.size());

	for(size_t loop = 0; loop < dataset->PlayerColours.size(); ++loop)
	{
		wxString Name = " "+FormatInt(loop)+" - "+GetPlayerColorName(loop);
		if(SearchMatches(Name.Lower()))
		{
			Colors_Colors_ListV->names.Add(Name);
			Colors_Colors_ListV->indexes.push_back(loop);
		}
		names.Add(Name);
	}

	virtualListing(Colors_Colors_ListV, &ColorIDs);

	short selection = Graphics_PlayerColor_ComboBox->GetSelection();
	Graphics_PlayerColor_ComboBox->Clear();
	Graphics_PlayerColor_ComboBox->Append("-1 - None");
	Graphics_PlayerColor_ComboBox->Append(names);
	Graphics_PlayerColor_ComboBox->SetSelection(selection);
}

void AGE_Frame::OnPlayerColorsSelect(wxCommandEvent &event)
{
    if(!colorTimer.IsRunning())
        colorTimer.Start(150);
}

void AGE_Frame::OnPlayerColorsTimer(wxTimerEvent&)
{
    colorTimer.Stop();
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
    wxBusyCursor WaitCursor;
    getSelectedItems(selections, Colors_Colors_ListV, ColorIDs);

    for(auto &box: uiGroupColor) box->clear();
    Colors_ID->clear();

	genie::PlayerColour * PlayerColorPointer = 0;
	for(auto loop = selections; loop--> 0;)
	{
		PlayerColorPointer = &dataset->PlayerColours[ColorIDs[loop]];

        Colors_ID->prepend(&PlayerColorPointer->ID);
        Colors_ColorL->prepend(&PlayerColorPointer->Colour);
        Colors_Unknown1->prepend(&PlayerColorPointer->Unknown1);
        Colors_Unknown2->prepend(&PlayerColorPointer->Unknown2);
		if(GenieVersion < genie::GV_AoKE3)	//	AoE and RoR
		{
			Colors_Name->prepend(&PlayerColorPointer->Name);
		}
		else	//	Above AoE and RoR
		{
			Colors_Palette->prepend(&PlayerColorPointer->Palette);
			Colors_MinimapColor->prepend(&PlayerColorPointer->MinimapColour);
			Colors_Unknown3->prepend(&PlayerColorPointer->Unknown3);
			Colors_Unknown4->prepend(&PlayerColorPointer->Unknown4);
			Colors_StatisticsText->prepend(&PlayerColorPointer->StatisticsText);
		}
	}
	SetStatusText("Selections: "+lexical_cast<string>(selections)+"    Selected color: "+lexical_cast<string>(ColorIDs.front()), 0);

    for(auto &box: uiGroupColor) box->update();
	Colors_ID->refill();
    if(PlayerColorPointer && !palettes.empty() && !palettes.front().empty())
    {
        genie::Color playerColor = palettes.front()[(uint8_t)PlayerColorPointer->Colour];
        genie::Color paletteStart = palettes.front()[(uint8_t)PlayerColorPointer->Palette];
        genie::Color minimap = palettes.front()[(uint8_t)PlayerColorPointer->MinimapColour];
        setForeAndBackColors(Colors_Palette, wxColour(paletteStart.r, paletteStart.g, paletteStart.b));
        setForeAndBackColors(Colors_ColorL, wxColour(playerColor.r, playerColor.g, playerColor.b));
        setForeAndBackColors(Colors_MinimapColor, wxColour(minimap.r, minimap.g, minimap.b));

		if(GenieVersion < genie::GV_AoKE3)
        AGE_SLP::playerColorStart = uint8_t(16 * (1 + ColorIDs.front()));
        else AGE_SLP::playerColorStart = (uint8_t)PlayerColorPointer->Palette;
        AGE_SLP::playerColorID = (uint8_t)PlayerColorPointer->Colour;
    }
}

void AGE_Frame::setForeAndBackColors(AGETextCtrl* box, wxColour color)
{
    box->SetBackgroundColour(color);
    if(color.Red() / 2 + 2 * color.Green() + color.Blue() > 384)
    box->SetForegroundColour(wxColour(0, 0, 0));
    else box->SetForegroundColour(wxColour(255, 255, 255));
}

void AGE_Frame::OnDrawPalette(wxPaintEvent &event)
{
    wxBufferedPaintDC dc(Colors_Palette_Display);
    dc.Clear();
    if(paletteView >= palettes.size() || palettes[paletteView].empty()) return;
    assert(palettes[paletteView].size() == 256);
    vector<uint8_t> rgbdata(768);
    uint8_t *val = rgbdata.data();
    for(int i=0; i < 256; ++i)
    {
        genie::Color rgba = palettes[paletteView][i];
        *val++ = rgba.r;
        *val++ = rgba.g;
        *val++ = rgba.b;
    }
    unsigned char *pic = (unsigned char*)rgbdata.data();
    wxBitmap bitmap = wxBitmap(wxImage(16, 16, pic, true).Scale(320, 320), 24);
    assert(bitmap.IsOk());
    dc.DrawBitmap(bitmap, 15, 15, true);
}

void AGE_Frame::OnPlayerColorsAdd(wxCommandEvent &event)
{
	if(!dataset) return;

	wxBusyCursor WaitCursor;
	AddToListIDFix(dataset->PlayerColours);
	ListPlayerColors();
}

void AGE_Frame::OnPlayerColorsInsert(wxCommandEvent &event)
{
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
	if(selections < 1) return;

	wxBusyCursor WaitCursor;
	InsertToListIDFix(dataset->PlayerColours, ColorIDs.front());
	ListPlayerColors();
}

void AGE_Frame::OnPlayerColorsDelete(wxCommandEvent &event)
{
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
	if(selections < 1) return;

	wxBusyCursor WaitCursor;
	DeleteFromListIDFix(dataset->PlayerColours, ColorIDs);
	ListPlayerColors();
}

void AGE_Frame::OnPlayerColorsCopy(wxCommandEvent &event)
{
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
	if(selections < 1) return;

	wxBusyCursor WaitCursor;
	CopyFromList(dataset->PlayerColours, ColorIDs, copies.PlayerColor);
	Colors_Colors_ListV->SetFocus();
    if(palettes.size())
    {
        paletteView = (paletteView + 1) % palettes.size();
        Colors_Palette_Display->Refresh();
    }
}

void AGE_Frame::OnPlayerColorsPaste(wxCommandEvent &event)
{
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
	if(selections < 1) return;

    wxBusyCursor WaitCursor;
    PasteToListIDFix(dataset->PlayerColours, ColorIDs, copies.PlayerColor);
    ListPlayerColors();
}

void AGE_Frame::OnPlayerColorsPasteInsert(wxCommandEvent &event)
{
	auto selections = Colors_Colors_ListV->GetSelectedItemCount();
	if(selections < 1) return;

	wxBusyCursor WaitCursor;
	PasteInsertToListIDFix(dataset->PlayerColours, ColorIDs.front(), copies.PlayerColor);
	ListPlayerColors();
}

void AGE_Frame::CreatePlayerColorControls()
{
	Colors_Main = new wxBoxSizer(wxHORIZONTAL);
	Tab_PlayerColors = new wxPanel(TabBar_Main);

	Colors_Colors = new wxStaticBoxSizer(wxVERTICAL, Tab_PlayerColors, "Player Colors");
	Colors_Colors_Search = new wxTextCtrl(Tab_PlayerColors, wxID_ANY);
	Colors_Colors_Search_R = new wxTextCtrl(Tab_PlayerColors, wxID_ANY);
	Colors_Colors_ListV = new AGEListView(Tab_PlayerColors, wxSize(200, 100));
	Colors_Colors_Buttons = new wxGridSizer(3, 0, 0);
	Colors_Add = new wxButton(Tab_PlayerColors, wxID_ANY, "Add", wxDefaultPosition, wxSize(10, -1));
	Colors_Insert = new wxButton(Tab_PlayerColors, wxID_ANY, "Insert New", wxDefaultPosition, wxSize(10, -1));
	Colors_Delete = new wxButton(Tab_PlayerColors, wxID_ANY, "Delete", wxDefaultPosition, wxSize(10, -1));
	Colors_Copy = new wxButton(Tab_PlayerColors, wxID_ANY, "Copy", wxDefaultPosition, wxSize(10, -1));
	Colors_Paste = new wxButton(Tab_PlayerColors, wxID_ANY, "Paste", wxDefaultPosition, wxSize(10, -1));
	Colors_PasteInsert = new wxButton(Tab_PlayerColors, wxID_ANY, "Ins Copies", wxDefaultPosition, wxSize(10, -1));

	Colors_DataArea = new wxBoxSizer(wxVERTICAL);
	Colors_WrapArea = new wxWrapSizer();
	Colors_Name_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_ID_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Palette_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Color_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_MinimapColor_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Unknown1_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Unknown2_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Unknown3_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Unknown4_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_StatisticsText_Holder = new wxBoxSizer(wxVERTICAL);
	Colors_Name_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Name", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_ID_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " ID", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Palette_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Palette *", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Color_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Color *", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_MinimapColor_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Minimap Color *", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Unknown1_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Unknown 1", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Unknown2_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Unknown 2", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Unknown3_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Unknown 3", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Unknown4_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Unknown 4", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_StatisticsText_Text = new wxStaticText(Tab_PlayerColors, wxID_ANY, " Statistics Text", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	Colors_Name = AGETextCtrl::init(CString, &uiGroupColor, this, &popUp, Tab_PlayerColors, 30);
	Colors_ID = AGETextCtrl::init(CLong, 0, this, &popUp, Tab_PlayerColors);
	Colors_Palette = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_Palette->SetToolTip("Starting index of the main color palette\nfrom where 8 colors are dedicated to this player color");
	Colors_ColorL = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_ColorL->SetToolTip("Index of the main color palette");
	Colors_MinimapColor = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_MinimapColor->SetToolTip("Index of the main color palette");
	Colors_Unknown1 = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_Unknown2 = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_Unknown3 = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_Unknown4 = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
	Colors_StatisticsText = AGETextCtrl::init(CLong, &uiGroupColor, this, &popUp, Tab_PlayerColors);
    Colors_Palette_Display = new wxPanel(Tab_PlayerColors, wxID_ANY, wxDefaultPosition, wxSize(256, 256));

	Colors_Colors_Buttons->Add(Colors_Add, 1, wxEXPAND);
	Colors_Colors_Buttons->Add(Colors_Delete, 1, wxEXPAND);
	Colors_Colors_Buttons->Add(Colors_Insert, 1, wxEXPAND);
	Colors_Colors_Buttons->Add(Colors_Copy, 1, wxEXPAND);
	Colors_Colors_Buttons->Add(Colors_Paste, 1, wxEXPAND);
	Colors_Colors_Buttons->Add(Colors_PasteInsert, 1, wxEXPAND);

	Colors_Colors->Add(Colors_Colors_Search, 0, wxEXPAND);
	Colors_Colors->Add(Colors_Colors_Search_R, 0, wxEXPAND);
	Colors_Colors->Add(Colors_Colors_ListV, 1, wxEXPAND | wxBOTTOM | wxTOP, 2);
	Colors_Colors->Add(Colors_Colors_Buttons, 0, wxEXPAND);

	Colors_Name_Holder->Add(Colors_Name_Text);
	Colors_Name_Holder->Add(Colors_Name, 0, wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxRIGHT, 5);
	Colors_ID_Holder->Add(Colors_ID_Text);
	Colors_ID_Holder->Add(Colors_ID, 0, wxEXPAND);
	Colors_Palette_Holder->Add(Colors_Palette_Text);
	Colors_Palette_Holder->Add(Colors_Palette, 0, wxEXPAND);
	Colors_Color_Holder->Add(Colors_Color_Text);
	Colors_Color_Holder->Add(Colors_ColorL, 0, wxEXPAND);
	Colors_MinimapColor_Holder->Add(Colors_MinimapColor_Text);
	Colors_MinimapColor_Holder->Add(Colors_MinimapColor, 0, wxEXPAND);
	Colors_Unknown1_Holder->Add(Colors_Unknown1_Text);
	Colors_Unknown1_Holder->Add(Colors_Unknown1, 0, wxEXPAND);
	Colors_Unknown2_Holder->Add(Colors_Unknown2_Text);
	Colors_Unknown2_Holder->Add(Colors_Unknown2, 0, wxEXPAND);
	Colors_Unknown3_Holder->Add(Colors_Unknown3_Text);
	Colors_Unknown3_Holder->Add(Colors_Unknown3, 0, wxEXPAND);
	Colors_Unknown4_Holder->Add(Colors_Unknown4_Text);
	Colors_Unknown4_Holder->Add(Colors_Unknown4, 0, wxEXPAND);
	Colors_StatisticsText_Holder->Add(Colors_StatisticsText_Text);
	Colors_StatisticsText_Holder->Add(Colors_StatisticsText, 0, wxEXPAND);

    Colors_DataArea->Add(Colors_Name_Holder, 0, wxTOP | wxRIGHT | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_ID_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Palette_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Color_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_MinimapColor_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Unknown1_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Unknown2_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Unknown3_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_Unknown4_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_WrapArea->Add(Colors_StatisticsText_Holder, 0, wxTOP | wxLEFT, 5);
    Colors_DataArea->Add(Colors_WrapArea, 0, wxTOP, 5);

	Colors_Main->Add(Colors_Colors, 1, wxEXPAND | wxTOP | wxLEFT | wxBOTTOM, 5);
	Colors_Main->Add(Colors_DataArea);
	Colors_Main->Add(Colors_Palette_Display, 2, wxEXPAND);

	if(EnableIDFix)
	Colors_ID->Enable(false);

	Tab_PlayerColors->SetSizer(Colors_Main);

	Connect(Colors_Colors_Search->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsSearch));
	Connect(Colors_Colors_Search_R->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsSearch));
	Connect(Colors_Colors_ListV->GetId(), wxEVT_COMMAND_LIST_ITEM_SELECTED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsSelect));
	Connect(Colors_Colors_ListV->GetId(), wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsSelect));
	Connect(Colors_Colors_ListV->GetId(), wxEVT_COMMAND_LIST_ITEM_FOCUSED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsSelect));
	Connect(Colors_Add->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsAdd));
	Connect(Colors_Insert->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsInsert));
	Connect(Colors_Delete->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsDelete));
	Connect(Colors_Copy->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsCopy));
	Connect(Colors_Paste->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsPaste));
	Connect(Colors_PasteInsert->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AGE_Frame::OnPlayerColorsPasteInsert));
    colorTimer.Bind(wxEVT_TIMER, &AGE_Frame::OnPlayerColorsTimer, this);
    Colors_Name->Bind(wxEVT_KILL_FOCUS, &AGE_Frame::OnKillFocus_Colors, this);
    Colors_Palette_Display->Bind(wxEVT_PAINT, &AGE_Frame::OnDrawPalette, this);
    Colors_Palette_Display->Bind(wxEVT_ERASE_BACKGROUND, &AGE_Frame::OnGraphicErase, this);
}

void AGE_Frame::OnKillFocus_Colors(wxFocusEvent &event)
{
	event.Skip();
	if(((AGETextCtrl*)event.GetEventObject())->SaveEdits() != 0) return;
	ListPlayerColors();
}
