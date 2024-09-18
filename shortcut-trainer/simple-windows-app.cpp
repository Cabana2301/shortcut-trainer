#include "window-base.h"

#include <stdexcept>
#include <chrono>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <random>

struct ShortcutRequirement {
	bool alt = false;
	bool shift = false;
	bool ctrl = false;
	uint8_t vk = 0;
	std::wstring description;
	bool eval(WPARAM wParam) {
		if (wParam != vk)
			return false;
		if (ctrl != bool(GetKeyState(VK_CONTROL) & 0x8000))
			return false;
		if (shift != bool(GetKeyState(VK_SHIFT) & 0x8000))
			return false;
		if (alt != bool(GetKeyState(VK_MENU) & 0x8000))
			return false;
		return true;
	}
};

struct Keys {
	Keys() {
		std::ifstream infile("virtual-keys.txt");
		if (!infile)
			throw std::runtime_error("virtual-keys.txt file not found!");
		for (std::string l; std::getline(infile, l);) {
			std::istringstream ss(l);
			std::string k;
			unsigned v;
			ss >> k;
			ss >> std::hex >> v;
			m[k] = v;
		}
	}
	std::unordered_map<std::string, unsigned> m;
};

void readShortcutFile(std::vector<ShortcutRequirement>* out) {
	Keys keys;
	std::ifstream infile("input.txt");
	if (!infile)
		throw std::runtime_error("input.txt file not found!");
	for (std::string l; std::getline(infile, l);) {
		std::istringstream ss(l);
		ShortcutRequirement shortcut;
		int i = 0;
		std::string word;
		while (ss >> word) {
			++i;
			if (word == "CTRL")
				shortcut.ctrl = true;
			else if (word == "ALT")
				shortcut.alt = true;
			else if (word == "SHIFT")
				shortcut.shift = true;
		}

		if (i) {
			auto it = keys.m.find(word);
			if (it == keys.m.end())
				throw std::runtime_error("Error in the shortcut file, unrecognized key");
			shortcut.vk = uint8_t(it->second);
			shortcut.description.assign(l.begin(), l.end()); // I know this is bad, don't judge me
			out->push_back(shortcut);
		}
	}
}


class MyWindow : public BaseWindow<MyWindow>
{
public:
	LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	PCWSTR  ClassName() const override;
private:
	class MyEditControl : public BaseWindow<MyEditControl> {
		using BaseWindow::BaseWindow;
		WNDPROC m_origFuncP = nullptr;

	public:
		PCWSTR  ClassName() const override {
			return L"EDIT";
		}
		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
			switch (uMsg) {
			case WM_KEYDOWN:
				if (!latest_command.empty()) {
					//if (wParam == 0x47 && (GetKeyState(VK_CONTROL) & 0x8000)) { // Ctrl+G
					if (key_combinations[selected_combo].eval(wParam)) {
						auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(clk::now() - measurement_start);
						auto t = std::div(dur.count(), std::uint64_t(1000));
						latest_command += L"took " +
							std::to_wstring(t.quot) + L"." +
							std::to_wstring(t.rem) + L" sec\r\n\r\n";
						display_text = latest_command + display_text;
						select_combo();
						SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)(latest_command + L"\r\n" + display_text).c_str());
						measurement_start = clk::now();
					}
					else {
						std::wostringstream display_stream;
						display_stream << std::hex << wParam
							<< L"  "
							<< latest_command
							<< L"\r\n"
							<< display_text;
						SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)(display_stream.str().c_str()));
					}
				}
				else {
					readShortcutFile(&key_combinations);
					re.seed(std::random_device{}());
					get_random.param(decltype(get_random)::param_type{ 0 , key_combinations.size() - 1 });
					select_combo();
					SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)(latest_command + L"\r\n" + display_text).c_str());
					measurement_start = clk::now();
				}

				break;
			}
			return CallWindowProc(m_origFuncP, m_hwnd, uMsg, wParam, lParam);
		}

		void setOrigFuncP(WNDPROC origFuncP) {
			m_origFuncP = origFuncP;
		}

	private:
		void select_combo() {
			selected_combo = get_random(re);
			latest_command = key_combinations[selected_combo].description + L"\r\n";
		}
	private:
		using clk = std::chrono::steady_clock;

		std::wstring display_text;
		std::wstring latest_command;
		std::chrono::time_point<clk> measurement_start;
		std::vector<ShortcutRequirement> key_combinations;
		std::default_random_engine re;
		std::uniform_int_distribution<size_t> get_random;
		int selected_combo = 0;
	};

	MyEditControl editControl;
};

PCWSTR  MyWindow::ClassName() const {
	return L"MyWindow";
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	MyWindow myWindow;
	myWindow.Create(L"Shortcut Trainer",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 800);
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

#define ID_EDITCHILD 100

LRESULT MyWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR lpszLatin[] =
		L"Press Any Key to start\r\n\r\n"
		L"Put the shortcuts that you want to practice in input.txt\r\n"
		L"You can see the list of key names in virtual-keys.txt\r\n"
		L"Make sure you have both files in your working directory\r\n"
		L"(usually where you have the exe file)\r\n"
		L"If you're running inside VisualStudio, don't worry about it.";

	switch (message)
	{
	case WM_CREATE: {
		editControl.CreateAsChild(
			WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0,
			0, 0, 0, 0,   // set size in WM_SIZE message 
			m_hwnd,       // parent window 
			(HMENU)ID_EDITCHILD,   // edit control ID 
			(HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)
		);
		// Need to do this because the base class will not handle it
		// The reason is that we can't RegisterClass the "EDIT" class
		// because it's built into Windows.
		SetWindowLongPtr(editControl.Window(), GWLP_USERDATA, (LONG_PTR)&editControl);

		// Now that the window is built, replace the handler function
		auto origFunc = SetWindowLongPtr(editControl.Window(),
			GWLP_WNDPROC,
			(LONG_PTR)MyEditControl::WindowProc);
		editControl.setOrigFuncP(reinterpret_cast<WNDPROC>(origFunc));

		// Add text to the window. 
		SendMessage(editControl.Window(), WM_SETTEXT, 0, (LPARAM)lpszLatin);
		SendMessage(editControl.Window(), EM_SETREADONLY, TRUE, 0);
		return 0;
	}

	case WM_COMMAND:
		return DefWindowProc(m_hwnd, message, wParam, lParam);

	case WM_SETFOCUS:
		SetFocus(editControl.Window());
		HideCaret(editControl.Window());
		return 0;

	case WM_SIZE:
		// Make the edit control the size of the window's client area. 

		MoveWindow(
			editControl.Window(),
			0, 0,                  // starting x- and y-coordinates 
			LOWORD(lParam),        // width of client area 
			HIWORD(lParam),        // height of client area 
			TRUE);                 // repaint window 
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(m_hwnd, message, wParam, lParam);
	}
	return NULL;
}
