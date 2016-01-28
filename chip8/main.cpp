#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
#include "chip8.h"

constexpr uint32_t kPixelSize = 10;
constexpr uint32_t kWindowWidth = kWidth * kPixelSize, kWindowHeight = kHeight * kPixelSize;

class Display : public sf::Drawable, public sf::Transformable {
public:
	Display() : vertices{sf::Quads, kWidth * kHeight * 4} {}

	void set_display(const std::bitset<kWidth*kHeight>& display) {
		for(int y = 0; y < kHeight; ++y) {
			for(int x = 0; x < kWidth; ++x) {
				int pixel = display[x + y * kWidth];
				sf::Color color = pixel ? sf::Color::White : sf::Color::Black;

				sf::Vertex* quad = &vertices[(x + y * kWidth) * 4];

				quad[0].color = color;
				quad[1].color = color;
				quad[2].color = color;
				quad[3].color = color;

				quad[0].position = sf::Vector2f(1.0f * x * kPixelSize, 1.0f * y * kPixelSize);
				quad[1].position = sf::Vector2f(1.0f * (x + 1) * kPixelSize, 1.0f * y * kPixelSize);
				quad[2].position = sf::Vector2f(1.0f * (x + 1) * kPixelSize, 1.0f * (y + 1) * kPixelSize);
				quad[3].position = sf::Vector2f(1.0f * x * kPixelSize, 1.0f * (y + 1) * kPixelSize);
			}
		}
	}

private:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
		states.transform *= getTransform();
		states.texture = nullptr;
		target.draw(vertices, states);
	}

	sf::VertexArray vertices;
};

int main(int argc, char *argv[]) try {
	if(argc < 2) {
		return 0;
	}
	Chip8 chip8;
	chip8.load(argv[1]);

	Display display;

	using k = sf::Keyboard;
	const int keymap[16]{
		k::X,k::Num1,k::Num1,k::Num1,
		k::Q,k::W,k::E,k::A,k::S,k::D,
		k::Z,k::C,k::Num4,k::R,k::F,k::V
	};

	sf::RenderWindow window{sf::VideoMode{kWindowWidth, kWindowHeight}, "Chip-8"};

	while(window.isOpen()) {
		sf::Event event;
		while(window.pollEvent(event)) {
			if(event.type == sf::Event::Closed) {
				window.close();
			} else if(event.type == sf::Event::KeyPressed ||
				event.type == sf::Event::KeyReleased) {
				for(int i = 0; i < 16; ++i) {
					if(event.key.code == keymap[i]) {
						chip8.set_key(i, event.type == sf::Event::KeyPressed);
						break;
					}
				}
			}
		}
		bool display_updated = false;
		for(int i = 0; i < 60 && !display_updated; ++i) {
			chip8.execute_instruction();
			display_updated = chip8.display_updated();
		}
		int sleep = 1;
		if(display_updated) {
			display.set_display(chip8.get_display());
			sleep = 16;
		}
		window.clear();
		window.draw(display);
		window.display();
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
	}
} catch(const exception::file_not_found &) {
	printf("file not found\n");
} catch(const exception::unknown_opcode &ex) {
	printf("unknown opcode: %#06x\n", ex.op.opcode);
}