
#!/usr/bin/env python3

import pygame
from pygame_button import Button
import time
import serial

frame = 1792, 896
pygame.display.set_caption('killer')
start_time = time.time() - 0.5

pygame.init()

class Button(object):
    """A fairly straight forward button class."""

    def __init__(self, rect, color, function, **kwargs):
        self.rect = pygame.Rect(rect)
        self.color = color
        self.function = function
        self.clicked = False
        self.hovered = False
        self.hover_text = None
        self.clicked_text = None
        self.process_kwargs(kwargs)
        self.render_text()

    def process_kwargs(self, kwargs):
        """Various optional customization you can change by passing kwargs."""
        settings = {
            "text": None,
            "font": pygame.font.Font(None, 16),
            "call_on_release": True,
            "hover_color": None,
            "clicked_color": None,
            "font_color": pygame.Color("white"),
            "hover_font_color": None,
            "clicked_font_color": None,
        }
        for kwarg in kwargs:
            if kwarg in settings:
                settings[kwarg] = kwargs[kwarg]
            else:
                raise AttributeError("Button has no keyword: {}".format(kwarg))
        self.__dict__.update(settings)

    def render_text(self):
        """Pre render the button text."""
        if self.text:
            if self.hover_font_color:
                color = self.hover_font_color
                self.hover_text = self.font.render(self.text, True, color)
            if self.clicked_font_color:
                color = self.clicked_font_color
                self.clicked_text = self.font.render(self.text, True, color)
            self.text = self.font.render(self.text, True, self.font_color)

    def check_event(self, event):
        """The button needs to be passed events from your program event loop."""
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            self.on_click(event)

    def on_click(self, event):
        if self.rect.collidepoint(event.pos):
            if not self.clicked:
                self.clicked = True
                if not self.call_on_release:
                    self.function()
            else:
                if self.clicked and self.call_on_release:
                    self.function()
                self.clicked = False

    def check_hover(self):
        if self.rect.collidepoint(pygame.mouse.get_pos()):
            if not self.hovered:
                self.hovered = True
        else:
            self.hovered = False

    def update(self, surface):
        """Update needs to be called every frame in the main loop."""
        color = self.color
        text = self.text
        self.check_hover()
        if self.clicked and self.clicked_color:
            color = self.clicked_color
            if self.clicked_font_color:
                text = self.clicked_text
        surface.fill(pygame.Color("black"), self.rect)
        surface.fill(color, self.rect.inflate(-4, -4))
        if self.text:
            text_rect = text.get_rect(center=self.rect.center)
            surface.blit(text, text_rect)


RED = (255, 0, 0)
BLUE = (0, 0, 255)
GREEN = (0, 255, 0)
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
ORANGE = (255, 180, 0)
buttongrid = 100
buttonsize = 90

BUTTON_STYLE = {
    "hover_color": BLUE,
    "clicked_color": GREEN,
    "clicked_font_color": BLACK,
    "hover_font_color": ORANGE,
}

up = Button((buttongrid*1, buttongrid*0, buttonsize, buttonsize), RED, lambda: None, text="up", **BUTTON_STYLE)
right = Button((buttongrid*2, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="right", **BUTTON_STYLE)
down = Button((buttongrid*1, buttongrid*2, buttonsize, buttonsize), RED, lambda: None, text="down", **BUTTON_STYLE)
left = Button((buttongrid*0, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="left", **BUTTON_STYLE)
select = Button((buttongrid*4, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="select", **BUTTON_STYLE)
start = Button((buttongrid*5, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="start", **BUTTON_STYLE)
a = Button((buttongrid*7, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="A", **BUTTON_STYLE)
b = Button((buttongrid*8, buttongrid*1, buttonsize, buttonsize), RED, lambda: None, text="B", **BUTTON_STYLE)

buttons = [up, right, down, left, select, start, a, b]

running = True
clock = True
# running = False


s = serial.Serial("/dev/ttyACM0", 9600)
def send_to_pi(clock, buttons):
    res = ""
    for i in buttons:
        if i.clicked:
            res += "1"
        else:
            res += "0"
    if clock:
        res += "1"
    else:
        res += "0"
    print(res)
    res += "\n"
    clock = not clock
    s.write(bytes(res, "UTF-8"))

if running:
    screen = pygame.display.set_mode(frame, pygame.RESIZABLE)
    pygame.init()

while running:
    current_time = time.time()
    elapsed_time = current_time - start_time

    for event in pygame.event.get():
        
        if event.type == pygame.QUIT:
            running = False

        if event.type == pygame.KEYDOWN:

            if event.key == pygame.K_ESCAPE:
                running = False
        
        for button in buttons:
            button.check_event(event)
    
    
        
    for button in buttons:
        button.update(screen)

    if elapsed_time > 0.1:

        pygame.display.update()
        screen.fill((0,0,0))

    if elapsed_time > 0.1:
        send_to_pi(clock, buttons)