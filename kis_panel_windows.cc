/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

// Panel has to be here to pass configure, so just test these
#if (defined(HAVE_LIBNCURSES) || defined (HAVE_LIBCURSES))

#include "kis_panel_widgets.h"
#include "kis_panel_frontend.h"
#include "kis_panel_windows.h"

Kis_Main_Panel::Kis_Main_Panel(GlobalRegistry *in_globalreg, 
							   KisPanelInterface *in_intf) : Kis_Panel(in_globalreg) {
	globalreg = in_globalreg;
	kpinterface = in_intf;

	menu = new Kis_Menu(globalreg);

	mn_file = menu->AddMenu("Kismet", 0);
	mi_connect = menu->AddMenuItem("Connect...", mn_file, 'C');
	menu->AddMenuItem("-", mn_file, 0);
	mi_quit = menu->AddMenuItem("Quit", mn_file, 'Q');

	mn_view = menu->AddMenu("Show", 1);
	mi_showtext = menu->AddMenuItem("Text", mn_view, 't');
	mi_showfields = menu->AddMenuItem("2-Fields", mn_view, '2');
	mi_showinput = menu->AddMenuItem("Input", mn_view, 'i');

	mn_sort = menu->AddMenu("Sort", 0);
	menu->AddMenuItem("Auto-fit", mn_sort, 'a');
	menu->AddMenuItem("-", mn_sort, 0);
	menu->AddMenuItem("Channel", mn_sort, 'c');
	menu->AddMenuItem("First Seen", mn_sort, 'f');
	menu->AddMenuItem("First Seen (descending)", mn_sort, 'F');
	menu->AddMenuItem("Latest Seen", mn_sort, 'l');
	menu->AddMenuItem("Latest Seen (descending)", mn_sort, 'L');
	menu->AddMenuItem("BSSID", mn_sort, 'b');
	menu->AddMenuItem("SSID", mn_sort, 's');
	menu->AddMenuItem("Packets", mn_sort, 'p');
	menu->AddMenuItem("Packets (descending)", mn_sort, 'P');

	// mn_tools = menu->AddMenu("Tools", 0);
	mn_cmds = menu->AddMenu("Cmds", 0);
	mi_addcard = menu->AddMenuItem("Add Source...", mn_cmds, 'A');

#if 0
	mn_tools = menu->AddMenu("Tools", 0);
	menu->DisableMenuItem(menu->AddMenuItem("Network List...", mn_tools, 'n'));
	menu->AddMenuItem("Client List...", mn_tools, 'c');
	menu->DisableMenuItem(menu->AddMenuItem("Details...", mn_tools, 'd'));

	menu->AddMenuItem("-", mn_tools, 0);

	menu->AddMenuItem("Server List...", mn_tools, 'S');
	menu->AddMenuItem("Capture Source List...", mn_tools, 'C');
	menu->AddMenuItem("GPS...", mn_tools, 'G');
#endif

	menu->Show();

	statustext = new Kis_Status_Text(globalreg);
	
	statuscli = new KisStatusText_Messageclient(globalreg, statustext);
	globalreg->messagebus->RegisterClient(statuscli, MSGFLAG_ALL);

	statustext->Show();

	active_component = statustext;
	comp_vec.push_back(statustext);

	SetTitle("Kismet Newcore Client");
}

Kis_Main_Panel::~Kis_Main_Panel() {
	globalreg->messagebus->RemoveClient(statuscli);

}

void Kis_Main_Panel::Position(int in_sy, int in_sx, int in_y, int in_x) {
	Kis_Panel::Position(in_sy, in_sx, in_y, in_x);

	menu->SetPosition(win, 1, 1, 0, 0);
	statustext->SetPosition(win, in_sx + 1, in_y - 7, in_x - 2, 5);
}

void Kis_Main_Panel::DrawPanel() {
	werase(win);

	DrawTitleBorder();

	for (unsigned int x = 0; x < comp_vec.size(); x++)
		comp_vec[x]->DrawComponent();

	menu->DrawComponent();

	wmove(win, 0, 0);
}

int Kis_Main_Panel::KeyPress(int in_key) {
	int ret;
	
	// Give the menu first shot, it'll ignore the key if it didn't have 
	// anything open.
	ret = menu->KeyPress(in_key);

	if (ret == 0) {
		// Menu ate the key, let it go
		return 0;
	}

	if (ret > 0) {
		// Menu processed an event, do something with it
		if (ret == mi_quit) {
			return -1;
		}

		if (ret == mi_connect) {
			Kis_Connect_Panel *cp = new Kis_Connect_Panel(globalreg, kpinterface);
			cp->Position((LINES / 2) - 4, (COLS / 2) - 20, 8, 40);
			globalreg->panel_interface->AddPanel(cp);
		}

		if (ret == mi_addcard) {
			vector<KisNetClient *> *cliref = kpinterface->FetchNetClientVecPtr();
			if (cliref->size() == 0) {
				kpinterface->RaiseAlert("No servers",
										"There are no servers.  You must\n"
										"connect to a server before adding\n"
										"cards.\n");
			} else if (cliref->size() == 1) {
				sp_addcard_cb(globalreg, kpinterface, (*cliref)[0], NULL);
			} else {
				kpinterface->RaiseServerPicker("Choose server", sp_addcard_cb,
											   NULL);
			}

		}

		return 0;
	}

	// Otherwise the menu didn't touch the key, so pass it to the top
	// component
	if (active_component != NULL) {
		ret = active_component->KeyPress(in_key);

		if (ret == 0)
			return 0;

		return ret;
	}

	return 0;
}

Kis_Connect_Panel::Kis_Connect_Panel(GlobalRegistry *in_globalreg, 
									 KisPanelInterface *in_intf) :
	Kis_Panel(in_globalreg) {
	globalreg = in_globalreg;
	kpinterface = in_intf;

	hostname = new Kis_Single_Input(globalreg);
	hostport = new Kis_Single_Input(globalreg);
	cancelbutton = new Kis_Button(globalreg);
	okbutton = new Kis_Button(globalreg);

	comp_vec.push_back(hostname);
	comp_vec.push_back(hostport);
	comp_vec.push_back(cancelbutton);
	comp_vec.push_back(okbutton);

	tab_components.push_back(hostname);
	tab_components.push_back(hostport);
	tab_components.push_back(okbutton);
	tab_components.push_back(cancelbutton);
	tab_pos = 0;

	active_component = hostname;

	SetTitle("Connect to Server");

	hostname->SetLabel("Host", LABEL_POS_LEFT);
	hostname->SetTextLen(120);
	hostname->SetCharFilter(FILTER_ALPHANUMSYM);

	hostport->SetLabel("Port", LABEL_POS_LEFT);
	hostport->SetTextLen(5);
	hostport->SetCharFilter(FILTER_NUM);

	okbutton->SetText("Connect");
	cancelbutton->SetText("Cancel");
}

Kis_Connect_Panel::~Kis_Connect_Panel() {
}

void Kis_Connect_Panel::Position(int in_sy, int in_sx, int in_y, int in_x) {
	Kis_Panel::Position(in_sy, in_sx, in_y, in_x);

	hostname->SetPosition(win, 2, 2, in_x - 6, 1);
	hostport->SetPosition(win, 2, 4, 14, 1);
	okbutton->SetPosition(win, in_x - 15, in_y - 2, 10, 1);
	cancelbutton->SetPosition(win, in_x - 15 - 2 - 15, in_y - 2, 10, 1);

	hostname->Activate(1);
	active_component = hostname;

	hostname->Show();
	hostport->Show();
	okbutton->Show();
	cancelbutton->Show();
}

void Kis_Connect_Panel::DrawPanel() {
	werase(win);

	DrawTitleBorder();

	for (unsigned int x = 0; x < comp_vec.size(); x++)
		comp_vec[x]->DrawComponent();

	wmove(win, 0, 0);
}

int Kis_Connect_Panel::KeyPress(int in_key) {
	int ret;

	// Rotate through the tabbed items
	if (in_key == '\t') {
		tab_components[tab_pos]->Deactivate();
		tab_pos++;
		if (tab_pos >= (int) tab_components.size())
			tab_pos = 0;
		tab_components[tab_pos]->Activate(1);
		active_component = tab_components[tab_pos];
	}

	// Otherwise the menu didn't touch the key, so pass it to the top
	// component
	if (active_component != NULL) {
		ret = active_component->KeyPress(in_key);

		if (active_component == okbutton && ret == 1) {
			if (hostname->GetText() == "")  {
				kpinterface->RaiseAlert("No hostname",
										"No hostname was provided for creating a\n"
										"new client connect to a Kismet server.\n"
										"A valid host name or IP is required.\n");
				return(0);
			}

			if (hostport->GetText() == "")  {
				kpinterface->RaiseAlert("No port",
										"No port number was provided for creating a\n"
										"new client connect to a Kismet server.\n"
										"A valid port number is required.\n");
				return(0);
			}
			
			// Try to add a client
			string clitxt = "tcp://" + hostname->GetText() + ":" +
				hostport->GetText();

			if (kpinterface->AddNetClient(clitxt, 1) < 0) 
				kpinterface->RaiseAlert("Connect failed", 
										"Failed to create new client connection\n"
										"to a Kismet server.  Check the status\n"
										"pane for more information about what\n"
										"went wrong.\n");

			globalreg->panel_interface->KillPanel(this);
		} else if (active_component == cancelbutton && ret == 1) {
			// Cancel and close
			globalreg->panel_interface->KillPanel(this);
		}
	}

	return 0;
}

Kis_ModalAlert_Panel::Kis_ModalAlert_Panel(GlobalRegistry *in_globalreg, 
										   KisPanelInterface *in_intf) :
	Kis_Panel(in_globalreg) {
	globalreg = in_globalreg;
	kpinterface = in_intf;

	ftxt = new Kis_Free_Text(globalreg);
	ackbutton = new Kis_Button(globalreg);

	comp_vec.push_back(ftxt);
	comp_vec.push_back(ackbutton);

	tab_components.push_back(ftxt);
	tab_components.push_back(ackbutton);
	tab_pos = 0;

	active_component = ftxt;

	SetTitle("");

	ackbutton->SetText("OK");
}

Kis_ModalAlert_Panel::~Kis_ModalAlert_Panel() {
}

void Kis_ModalAlert_Panel::Position(int in_sy, int in_sx, int in_y, int in_x) {
	Kis_Panel::Position(in_sy, in_sx, in_y, in_x);

	ftxt->SetPosition(win, 1, 1, in_x - 2, in_y - 3);
	ackbutton->SetPosition(win, (in_x / 2) - 7, in_y - 2, 14, 1);

	ftxt->Activate(1);
	active_component = ftxt;

	ftxt->Show();
	ackbutton->Show();
}

void Kis_ModalAlert_Panel::DrawPanel() {
	werase(win);

	DrawTitleBorder();

	for (unsigned int x = 0; x < comp_vec.size(); x++)
		comp_vec[x]->DrawComponent();

	wmove(win, 0, 0);
}

int Kis_ModalAlert_Panel::KeyPress(int in_key) {
	int ret;

	// Rotate through the tabbed items
	if (in_key == '\t') {
		tab_components[tab_pos]->Deactivate();
		tab_pos++;
		if (tab_pos >= (int) tab_components.size())
			tab_pos = 0;
		tab_components[tab_pos]->Activate(1);
		active_component = tab_components[tab_pos];
	}

	// Otherwise the menu didn't touch the key, so pass it to the top
	// component
	if (active_component != NULL) {
		ret = active_component->KeyPress(in_key);

		if (active_component == ackbutton && ret == 1) {
			// We're done
			globalreg->panel_interface->KillPanel(this);
		}
	}

	return 0;
}

void Kis_ModalAlert_Panel::ConfigureAlert(string in_title, string in_text) {
	SetTitle(in_title);
	ftxt->SetText(in_text);
}

Kis_ServerList_Picker::Kis_ServerList_Picker(GlobalRegistry *in_globalreg, 
											 KisPanelInterface *in_intf) :
	Kis_Panel(in_globalreg) {
	globalreg = in_globalreg;
	kpinterface = in_intf;

	// Grab the pointer to the list of clients maintained
	netcliref = kpinterface->FetchNetClientVecPtr();

	srvlist = new Kis_Scrollable_Table(globalreg);

	comp_vec.push_back(srvlist);

	// TODO -- Add name parsing to KISMET proto in netclient, add support here
	vector<Kis_Scrollable_Table::title_data> titles;
	Kis_Scrollable_Table::title_data t;
	t.width = 16;
	t.title = "Host";
	t.alignment = 0;
	titles.push_back(t);
	t.width = 5;
	t.title = "Port";
	t.alignment = 2;
	titles.push_back(t);
	t.width = 4;
	t.title = "Cntd";
	t.alignment = 0;
	titles.push_back(t);
	t.width = 3;
	t.title = "Rdy";
	t.alignment = 0;
	titles.push_back(t);
	srvlist->AddTitles(titles);

	// Population is done during draw

	active_component = srvlist;

	srvlist->Activate(1);

	SetTitle("");

	cb_hook = NULL;
	cb_aux = NULL;
}

Kis_ServerList_Picker::~Kis_ServerList_Picker() {
}

void Kis_ServerList_Picker::Position(int in_sy, int in_sx, int in_y, int in_x) {
	Kis_Panel::Position(in_sy, in_sx, in_y, in_x);

	srvlist->SetPosition(win, 1, 1, in_x - 2, in_y - 2);

	srvlist->Show();
}

void Kis_ServerList_Picker::DrawPanel() {
	werase(win);

	DrawTitleBorder();

	// Grab the list of servers and populate with it.  We'll assume that the number
	// of servers, and their order, cannot change while we're in the picker, since
	// the user can't get at it.  We WILL have to handle updating the connection
	// status based on the position key.  This is NOT A SAFE ASSUMPTION for any other
	// of the picker types (like cards), so don't blind-copy this code later.
	vector<string> td;
	ostringstream osstr;
	for (unsigned int x = 0; x < netcliref->size(); x++) {
		td.clear();

		td.push_back((*netcliref)[x]->FetchHost());

		osstr << (*netcliref)[x]->FetchPort();
		td.push_back(osstr.str());
		osstr.str("");

		if ((*netcliref)[x]->Valid()) {
			td.push_back("Yes");
			if ((*netcliref)[x]->FetchConfigured() < 0)
				td.push_back("Tes");
			else
				td.push_back("No");
		} else {
			td.push_back("No");
			td.push_back("No");
		}

		srvlist->ReplaceRow(x, td);
	}


	for (unsigned int x = 0; x < comp_vec.size(); x++)
		comp_vec[x]->DrawComponent();

	wmove(win, 0, 0);
}

int Kis_ServerList_Picker::KeyPress(int in_key) {
	int ret;
	int listkey;
	
	// Rotate through the tabbed items
	if (in_key == '\n' || in_key == '\r') {
		listkey = srvlist->GetSelected();

		// Sanity check, even though nothing should be able to change this
		// while we're open since we claim the input.
		// We could raise an alert but theres nothing the user could do 
		// about it so we'll just silently close the window
		if (listkey >= 0 && listkey < (int) netcliref->size()) {
			(*cb_hook)(globalreg, kpinterface, (*netcliref)[listkey], cb_aux);
		}

		globalreg->panel_interface->KillPanel(this);
	}

	// Otherwise the menu didn't touch the key, so pass it to the top
	// component
	if (active_component != NULL) {
		ret = active_component->KeyPress(in_key);
	}

	return 0;
}

void Kis_ServerList_Picker::ConfigurePicker(string in_title, kpi_sl_cb_hook in_hook,
											void *in_aux) {
	SetTitle(in_title);
	cb_hook = in_hook;
	cb_aux = in_aux;
}

// Addcard callback is used to actually build the addcard window once
// we've picked a source.  This will be called directly from the main
// menu handlers if there aren't any sources.
void sp_addcard_cb(KPI_SL_CB_PARMS) {
	Kis_AddCard_Panel *acp = new Kis_AddCard_Panel(globalreg, kpi);

	acp->Position((LINES / 2) - 5, (COLS / 2) - 17, 10, 34);

	acp->SetTargetClient(picked);

	kpi->AddPanel(acp);
}

Kis_AddCard_Panel::Kis_AddCard_Panel(GlobalRegistry *in_globalreg, 
									 KisPanelInterface *in_intf) :
	Kis_Panel(in_globalreg) {
	globalreg = in_globalreg;
	kpinterface = in_intf;

	srctype = new Kis_Single_Input(globalreg);
	srciface = new Kis_Single_Input(globalreg);
	srcname = new Kis_Single_Input(globalreg);

	okbutton = new Kis_Button(globalreg);
	cancelbutton = new Kis_Button(globalreg);

	comp_vec.push_back(srctype);
	comp_vec.push_back(srciface);
	comp_vec.push_back(srcname);
	comp_vec.push_back(okbutton);
	comp_vec.push_back(cancelbutton);

	tab_components.push_back(srctype);
	tab_components.push_back(srciface);
	tab_components.push_back(srcname);
	tab_components.push_back(okbutton);
	tab_components.push_back(cancelbutton);
	tab_pos = 0;

	active_component = srctype;

	SetTitle("Add Source");

	srctype->SetLabel("Type", LABEL_POS_LEFT);
	srctype->SetTextLen(32);
	srctype->SetCharFilter(FILTER_ALPHANUMSYM);

	srciface->SetLabel("Intf", LABEL_POS_LEFT);
	srciface->SetTextLen(32);
	srciface->SetCharFilter(FILTER_ALPHANUMSYM);
		
	srcname->SetLabel("Name", LABEL_POS_LEFT);
	srcname->SetTextLen(32);
	srcname->SetCharFilter(FILTER_ALPHANUMSYM);

	okbutton->SetText("Add");
	cancelbutton->SetText("Cancel");

	target_cli = NULL;
}

Kis_AddCard_Panel::~Kis_AddCard_Panel() {
}

void Kis_AddCard_Panel::Position(int in_sy, int in_sx, int in_y, int in_x) {
	Kis_Panel::Position(in_sy, in_sx, in_y, in_x);

	srctype->SetPosition(win, 2, 2, in_x - 6, 1);
	srciface->SetPosition(win, 2, 4, in_x - 15, 1);
	srcname->SetPosition(win, 2, 6, in_x - 6, 1);
	okbutton->SetPosition(win, in_x - 15, in_y - 2, 10, 1);
	cancelbutton->SetPosition(win, in_x - 15 - 2 - 15, in_y - 2, 10, 1);

	srctype->Activate(1);
	active_component = srctype;

	srctype->Show();
	srciface->Show();
	srcname->Show();
	
	okbutton->Show();
	cancelbutton->Show();
}

void Kis_AddCard_Panel::SetTargetClient(KisNetClient *in_cli) {
	target_cli = in_cli;

	ostringstream osstr;
	osstr << "Add Source to " << in_cli->FetchHost() << ":" << in_cli->FetchPort();

	SetTitle(osstr.str());
}

void Kis_AddCard_Panel::DrawPanel() {
	werase(win);

	DrawTitleBorder();

	for (unsigned int x = 0; x < comp_vec.size(); x++)
		comp_vec[x]->DrawComponent();

	wmove(win, 0, 0);
}

int Kis_AddCard_Panel::KeyPress(int in_key) {
	int ret;

	// Rotate through the tabbed items
	if (in_key == '\t') {
		tab_components[tab_pos]->Deactivate();
		tab_pos++;
		if (tab_pos >= (int) tab_components.size())
			tab_pos = 0;
		tab_components[tab_pos]->Activate(1);
		active_component = tab_components[tab_pos];
	}

	// Otherwise the menu didn't touch the key, so pass it to the top
	// component
	if (active_component != NULL) {
		ret = active_component->KeyPress(in_key);

		if (active_component == okbutton && ret == 1) {
			if (srctype->GetText() == "") {
				kpinterface->RaiseAlert("No source type",
										"No source type was provided for\n"
										"creating a new source.  A source\n"
										"type is required.\n");
				return(0);
			}

			if (srciface->GetText() == "") {
				kpinterface->RaiseAlert("No source interface",
										"No source interface was provided for\n"
										"creating a new source.  A source\n"
										"interface is required.\n");
				return(0);
			}

			if (srcname->GetText() == "") {
				kpinterface->RaiseAlert("No source name",
										"No source name was provided for\n"
										"creating a new source.  A source name\n"
										"is required.\n");
				return(0);
			}

			if (target_cli == NULL) {
				globalreg->panel_interface->KillPanel(this);
				return(0);
			}

			if (target_cli->Valid() == 0) {
				kpinterface->RaiseAlert("Server unavailable",
										"The selected server is not available.\n");
				return(0);
			}

			// Build a command and inject it
			string srccmd;
			srccmd = "ADDSOURCE " + srctype->GetText() + "," +
				srciface->GetText() + "," + srcname->GetText();

			target_cli->InjectCommand(srccmd);

			globalreg->panel_interface->KillPanel(this);
		} else if (active_component == cancelbutton && ret == 1) {
			// Cancel and close
			globalreg->panel_interface->KillPanel(this);
		}
	}

	return 0;
}

#endif

