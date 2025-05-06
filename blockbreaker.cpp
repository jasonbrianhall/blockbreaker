// BlockBreaker - A simple block breaking game using C++, GTK3, and Cairo
// 
// Compile with:
// g++ -o block_breaker block_breaker.cpp `pkg-config --cflags --libs gtk+-3.0`

#include <gtk/gtk.h>
#include <cairo.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <memory>
#include <iostream>

// Game constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PADDLE_WIDTH = 100;
const int PADDLE_HEIGHT = 20;
const int BALL_RADIUS = 10;
const int BLOCK_WIDTH = 80;
const int BLOCK_HEIGHT = 30;
const int BLOCK_ROWS = 5;
const int BLOCK_COLS = 9;
const int BLOCK_SPACING = 5;
const int TOP_MARGIN = 50;
const int SIDE_MARGIN = 20;
const double BALL_SPEED = 5.0;

// Game objects
struct Ball {
    double x, y;
    double dx, dy;
    int radius;
    
    Ball(double startX, double startY, int r) : x(startX), y(startY), radius(r) {
        // Initial direction: upward at an angle
        double angle = M_PI / 4.0;  // 45 degrees
        dx = BALL_SPEED * cos(angle);
        dy = -BALL_SPEED * sin(angle);
    }
    
    void move() {
        x += dx;
        y += dy;
    }
    
    void draw(cairo_t* cr) {
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);  // Yellow color
        cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
        cairo_fill(cr);
    }
};

struct Paddle {
    double x, y;
    int width, height;
    
    Paddle(double startX, double startY, int w, int h) 
        : x(startX), y(startY), width(w), height(h) {}
    
    void draw(cairo_t* cr) {
        cairo_set_source_rgb(cr, 0.0, 0.7, 1.0);  // Blue color
        cairo_rectangle(cr, x - width / 2, y - height / 2, width, height);
        cairo_fill(cr);
    }
    
    void move(double newX) {
        // Ensure paddle stays within window bounds
        if (newX - width / 2 < 0) {
            x = width / 2;
        } else if (newX + width / 2 > WINDOW_WIDTH) {
            x = WINDOW_WIDTH - width / 2;
        } else {
            x = newX;
        }
    }
};

struct Block {
    double x, y;
    int width, height;
    bool active;
    double r, g, b;  // Color
    
    Block(double startX, double startY, int w, int h) 
        : x(startX), y(startY), width(w), height(h), active(true) {
        // Assign a random color
        r = 0.3 + (rand() % 70) / 100.0;
        g = 0.3 + (rand() % 70) / 100.0;
        b = 0.3 + (rand() % 70) / 100.0;
    }
    
    void draw(cairo_t* cr) {
        if (!active) return;
        
        // Create a gradient for 3D effect
        cairo_pattern_t *gradient = cairo_pattern_create_linear(x, y, x + width, y + height);
        cairo_pattern_add_color_stop_rgb(gradient, 0.0, r * 1.2 > 1.0 ? 1.0 : r * 1.2, 
                                                     g * 1.2 > 1.0 ? 1.0 : g * 1.2, 
                                                     b * 1.2 > 1.0 ? 1.0 : b * 1.2);  // Lighter top-left
        cairo_pattern_add_color_stop_rgb(gradient, 1.0, r * 0.7, g * 0.7, b * 0.7);  // Darker bottom-right
        
        // Draw block main body with gradient
        cairo_rectangle(cr, x, y, width, height);
        cairo_set_source(cr, gradient);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(gradient);
        
        // Add highlight on top and left edges
        cairo_set_source_rgba(cr, 1, 1, 1, 0.5);  // Brighter light border
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, x, y + height);
        cairo_line_to(cr, x, y);
        cairo_line_to(cr, x + width, y);
        cairo_stroke(cr);
        
        // Add shadow on bottom and right edges
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);  // Darker shadow border
        cairo_move_to(cr, x + width, y);
        cairo_line_to(cr, x + width, y + height);
        cairo_line_to(cr, x, y + height);
        cairo_stroke(cr);
        
        // Add inner bevel for extra 3D effect
        cairo_set_source_rgba(cr, r * 0.8, g * 0.8, b * 0.8, 1.0);
        cairo_rectangle(cr, x + 3, y + 3, width - 6, height - 6);
        cairo_fill(cr);
    }
};

// Game class
class BlockBreakerGame {
private:
    std::unique_ptr<Ball> ball;
    std::unique_ptr<Paddle> paddle;
    std::vector<Block> blocks;
    bool gameRunning;
    bool gameOver;
    int score;
    int lives;
    
public:
    BlockBreakerGame() : gameRunning(false), gameOver(false), score(0), lives(3) {
        resetGame();
    }
    
    void resetGame() {
        // Initialize ball
        ball = std::make_unique<Ball>(WINDOW_WIDTH / 2, WINDOW_HEIGHT - 50, BALL_RADIUS);
        
        // Initialize paddle
        paddle = std::make_unique<Paddle>(WINDOW_WIDTH / 2, WINDOW_HEIGHT - 30, PADDLE_WIDTH, PADDLE_HEIGHT);
        
        // Initialize blocks
        blocks.clear();
        for (int row = 0; row < BLOCK_ROWS; row++) {
            for (int col = 0; col < BLOCK_COLS; col++) {
                double blockX = SIDE_MARGIN + col * (BLOCK_WIDTH + BLOCK_SPACING);
                double blockY = TOP_MARGIN + row * (BLOCK_HEIGHT + BLOCK_SPACING);
                blocks.emplace_back(blockX, blockY, BLOCK_WIDTH, BLOCK_HEIGHT);
            }
        }
        
        gameRunning = false;
        gameOver = false;
    }
    
    void start() {
        gameRunning = true;
    }
    
    void movePaddle(double x) {
        paddle->move(x);
        
        // If game hasn't started, move the ball with the paddle
        if (!gameRunning && !gameOver) {
            ball->x = paddle->x;
        }
    }
    
    bool update() {
        if (!gameRunning || gameOver) return true;
        
        // Move the ball
        ball->move();
        
        // Check for collisions with walls
        if (ball->x - ball->radius <= 0 || ball->x + ball->radius >= WINDOW_WIDTH) {
            ball->dx = -ball->dx;  // Reverse horizontal direction
        }
        
        if (ball->y - ball->radius <= 0) {
            ball->dy = -ball->dy;  // Reverse vertical direction
        }
        
        // Check for collision with paddle
        if (ball->y + ball->radius >= paddle->y - paddle->height / 2 &&
            ball->y - ball->radius <= paddle->y + paddle->height / 2 &&
            ball->x >= paddle->x - paddle->width / 2 &&
            ball->x <= paddle->x + paddle->width / 2) {
            
            // Calculate reflection angle based on where the ball hit the paddle
            double hitPos = (ball->x - paddle->x) / (paddle->width / 2);  // -1 to 1
            double angle = hitPos * (M_PI / 3);  // -60 to 60 degrees
            
            ball->dy = -std::abs(ball->dy);  // Always bounce upward
            
            // Adjust horizontal direction based on hit position
            double speed = std::sqrt(ball->dx * ball->dx + ball->dy * ball->dy);
            ball->dx = speed * std::sin(angle);
            ball->dy = -speed * std::cos(angle);
        }
        
        // Check for collisions with blocks
        for (auto& block : blocks) {
            if (!block.active) continue;
            
            bool collision = false;
            double collisionSide = 0; // 0=top, 1=right, 2=bottom, 3=left
            double overlapX = 0, overlapY = 0;
            
            // Calculate the closest point on the block to the ball
            double closestX = std::max(block.x, std::min(ball->x, block.x + block.width));
            double closestY = std::max(block.y, std::min(ball->y, block.y + block.height));
            
            // Calculate the distance between the ball and the closest point
            double distanceX = ball->x - closestX;
            double distanceY = ball->y - closestY;
            double distanceSquared = distanceX * distanceX + distanceY * distanceY;
            
            // Check if the distance is less than the ball's radius
            if (distanceSquared < ball->radius * ball->radius) {
                collision = true;
                
                // Determine which side of the block was hit
                // Based on the ball's position and velocity
                double ball_right = ball->x + ball->radius;
                double ball_left = ball->x - ball->radius;
                double ball_top = ball->y - ball->radius;
                double ball_bottom = ball->y + ball->radius;
                double block_right = block.x + block.width;
                double block_left = block.x;
                double block_top = block.y;
                double block_bottom = block.y + block.height;
                
                // Calculate time of collision with each side
                double t_left = (ball->dx > 0) ? (block_left - ball_right) / ball->dx : INFINITY;
                double t_right = (ball->dx < 0) ? (block_right - ball_left) / ball->dx : INFINITY;
                double t_top = (ball->dy > 0) ? (block_top - ball_bottom) / ball->dy : INFINITY;
                double t_bottom = (ball->dy < 0) ? (block_bottom - ball_top) / ball->dy : INFINITY;
                
                // Find the earliest collision
                double t = std::min(std::min(t_left, t_right), std::min(t_top, t_bottom));
                
                if (t == t_left) collisionSide = 3;
                else if (t == t_right) collisionSide = 1;
                else if (t == t_top) collisionSide = 0;
                else collisionSide = 2;
                
                // Alternative approach if the above time-based approach is unstable
                // Determine impact side by checking where the closest point is on the block
                if (closestX == block.x) collisionSide = 3; // Left side
                else if (closestX == block.x + block.width) collisionSide = 1; // Right side
                else if (closestY == block.y) collisionSide = 0; // Top side
                else collisionSide = 2; // Bottom side
            }
            
            if (collision) {
                block.active = false;
                score += 10;
                
                // Change direction based on which side was hit
                switch (static_cast<int>(collisionSide)) {
                    case 0: // Top
                    case 2: // Bottom
                        ball->dy = -ball->dy;
                        // Add a slight random horizontal angle variation to make gameplay more interesting
                        ball->dx += ((rand() % 100) / 500.0) - 0.1;
                        break;
                    case 1: // Right
                    case 3: // Left
                        ball->dx = -ball->dx;
                        // Add a slight random vertical angle variation
                        ball->dy += ((rand() % 100) / 500.0) - 0.1;
                        break;
                }
                
                // Normalize speed to keep it consistent
                double speed = std::sqrt(ball->dx * ball->dx + ball->dy * ball->dy);
                ball->dx = (ball->dx / speed) * BALL_SPEED;
                ball->dy = (ball->dy / speed) * BALL_SPEED;
                
                break;  // Only one collision per frame
            }
        }
        
        // Check if ball falls below the screen
        if (ball->y - ball->radius > WINDOW_HEIGHT) {
            lives--;
            if (lives <= 0) {
                gameOver = true;
            } else {
                // Reset ball position
                ball->x = paddle->x;
                ball->y = WINDOW_HEIGHT - 50;
                ball->dx = BALL_SPEED * cos(M_PI / 4.0);
                ball->dy = -BALL_SPEED * sin(M_PI / 4.0);
                gameRunning = false;
            }
        }
        
        // Check if all blocks are destroyed
        bool allBlocksDestroyed = true;
        for (const auto& block : blocks) {
            if (block.active) {
                allBlocksDestroyed = false;
                break;
            }
        }
        
        if (allBlocksDestroyed) {
            gameOver = true;  // Player wins
        }
        
        return true;
    }
    
    void draw(cairo_t* cr) {
        // Draw background
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.2);  // Dark blue/black
        cairo_paint(cr);
        
        // Draw blocks
        for (auto& block : blocks) {
            block.draw(cr);
        }
        
        // Draw paddle
        paddle->draw(cr);
        
        // Draw ball
        ball->draw(cr);
        
        // Draw score and lives
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 20);
        
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
        cairo_move_to(cr, 20, 30);
        cairo_show_text(cr, scoreText);
        
        char livesText[50];
        snprintf(livesText, sizeof(livesText), "Lives: %d", lives);
        cairo_move_to(cr, WINDOW_WIDTH - 100, 30);
        cairo_show_text(cr, livesText);
        
        // Draw game status message
        if (!gameRunning && !gameOver) {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7);
            cairo_rectangle(cr, WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 30, 300, 60);
            cairo_fill(cr);
            
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 24);
            cairo_move_to(cr, WINDOW_WIDTH / 2 - 140, WINDOW_HEIGHT / 2 + 10);
            cairo_show_text(cr, "Click to Start!");
        } else if (gameOver) {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7);
            cairo_rectangle(cr, WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 30, 300, 60);
            cairo_fill(cr);
            
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 24);
            
            if (lives <= 0) {
                cairo_move_to(cr, WINDOW_WIDTH / 2 - 140, WINDOW_HEIGHT / 2 + 10);
                cairo_show_text(cr, "Game Over!");
            } else {
                cairo_move_to(cr, WINDOW_WIDTH / 2 - 140, WINDOW_HEIGHT / 2 + 10);
                cairo_show_text(cr, "You Win!");
            }
            
            cairo_set_font_size(cr, 18);
            cairo_move_to(cr, WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 + 40);
            cairo_show_text(cr, "Click to Play Again");
        }
    }
    
    bool isGameRunning() const {
        return gameRunning;
    }
    
    bool isGameOver() const {
        return gameOver;
    }
};

// GTK application
BlockBreakerGame game;
GtkWidget* drawingArea;

// Drawing callback
static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    game.draw(cr);
    return FALSE;
}

// Timer callback for game loop
static gboolean on_timeout(gpointer user_data) {
    game.update();
    gtk_widget_queue_draw(drawingArea);
    return G_SOURCE_CONTINUE;
}

// Mouse motion callback
static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer user_data) {
    game.movePaddle(event->x);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

// Mouse click callback
static gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    if (!game.isGameRunning()) {
        if (game.isGameOver()) {
            game.resetGame();
        }
        game.start();
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

int main(int argc, char** argv) {
    // Initialize random number generator
    srand(time(nullptr));
    
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create window
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Block Breaker");
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create drawing area
    drawingArea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawingArea);
    
    // Connect signals
    g_signal_connect(drawingArea, "draw", G_CALLBACK(on_draw), NULL);
    
    // Add mouse event handling
    gtk_widget_add_events(drawingArea, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(drawingArea, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(drawingArea, "button-press-event", G_CALLBACK(on_button_press), NULL);
    
    // Show window
    gtk_widget_show_all(window);
    
    // Start game timer (60 FPS)
    g_timeout_add(1000 / 60, on_timeout, NULL);
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}
