﻿#include "Sudoku.h"

Sudoku::Sudoku::Sudoku()
	: mWindowHeight(880), mWindowWidth(720),
	  mGridHeight(720), mGridWidth(720),
	  mGridRows(9), mGridCols(9),
	  mWindow(nullptr), mRenderer(nullptr), 
	  mTotalTextures(14), mTextureCache{ nullptr },
	  mFont(nullptr), mFontSize(mGridHeight/12),
	  mTotalCells(81),
	  mClearColour({ 0, 0, 0, SDL_ALPHA_OPAQUE })
{

}

Sudoku::Sudoku::~Sudoku()
{
	freeTextures();
	close();
}

bool Sudoku::Sudoku::initialiseSDL()
{
	// Set success initialisation flag
	bool success = true;

	// Initalise SDL video subsystem
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout << "SDL could not intialise! Error: " << SDL_GetError() << std::endl;
		success = false;
	}

	// Initialise SDL_ttf
	if (TTF_Init() == -1)
	{
		std::cout << "SDL_ttf could not initialise! Error: " << TTF_GetError() << std::endl;
		success = false;
	}

	// Initialise SDL audio subsystem
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		std::cerr << "SDL audio could not initialize! Error: " << SDL_GetError() << std::endl;
		success = false;
	}

	// Initialise SDL_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cerr << "SDL_mixer coud not initialize! Error: " << Mix_GetError() << std::endl;
		success = false;
	}


	// Create window
	mWindow = SDL_CreateWindow("Sudoku", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mWindowWidth, mWindowHeight, SDL_WINDOW_SHOWN);
	if (mWindow == nullptr)
	{
		std::cout << "SDL could not create window! Error: " << SDL_GetError() << std::endl;
		success = false;
	}

	// Create renderer
	mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
	if (mRenderer == nullptr)
	{
		std::cout << "SDL could not create renderer! Error: " << SDL_GetError() << std::endl;
		success = false;
	}

	// Load font for text
	mFont = TTF_OpenFont("assets/octin sports free.ttf", mFontSize);
	if (mFont == nullptr)
	{
		std::cout << "Failed to load font! Error: " << TTF_GetError() << std::endl;
		success = false;
	}

	// Load sound effect
	soundEffect = Mix_LoadWAV("assets/effect.wav");
	if (!soundEffect) {
		std::cout << "Failed to load soundEffect! Error: " << Mix_GetError() << std::endl;
		success = false;
	}

	// Load music theme
	music = Mix_LoadMUS("assets/loop_music.mp3");
	if (!music) {
		std::cout << "Failed to load music! Error: " << Mix_GetError() << std::endl;
		success = false;
	}
	// Load new game effect
	newLevelEffect = Mix_LoadWAV("assets/new_game.wav");
	Mix_VolumeChunk(newLevelEffect, 50);
	if (!newLevelEffect) {
		std::cout << "Failed to load new level soundeffect! Error: " << Mix_GetError() << std::endl;
		success = false;
	}

	winSoundEffect = Mix_LoadWAV("assets/win.wav");
	if (!winSoundEffect) {
		std::cout << "Failed to load  winSoundeffect! Error: " << Mix_GetError() << std::endl;
		success = false;
	}

	checkSolutionSoundEffect = Mix_LoadWAV("assets/checksolution.wav");
	Mix_VolumeChunk(checkSolutionSoundEffect, 15);
	if (!checkSolutionSoundEffect) {
		std::cout << "Failed to load sound effect to check solution! Error: " << Mix_GetError() << std::endl;
		success = false;
	}


	return success;
}

inline int Sudoku::Sudoku::getIndex(int row, int col) const
{
	return row * mGridRows + col;
}

void Sudoku::Sudoku::loadTexture(SDL_Texture*& texture, const char* text, SDL_Color& fontColour)
{
	// Create text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(mFont, text, fontColour);
	if (textSurface == nullptr)
	{
		std::cout << "Could not create TTF SDL_Surface! Error: " << TTF_GetError() << std::endl;
	}
	else
	{
		// Create texture from surface pixels
		texture = SDL_CreateTextureFromSurface(mRenderer, textSurface);
		if (texture == nullptr)
		{
			std::cout << "Could not create texture from surface! Error: " << SDL_GetError() << std::endl;
		}
		SDL_FreeSurface(textSurface);
	}
}

void Sudoku::Sudoku::preloadTextures()
{
	// Choose colour of font
	SDL_Color fontColour = { 0, 0, 0, SDL_ALPHA_OPAQUE }; // black

	// Load texture for empty space
	loadTexture(mTextureCache[0], " ", fontColour);

	// Load textures for numbers from 1 to 9
	for (int num = 1; num < 10; num++)
	{
		const char temp[] = { '0' + num, '\0' };
		loadTexture(mTextureCache[num], temp, fontColour);
	}

	// Load texture for "Check", "Solve", and "New" buttons
	loadTexture(mTextureCache[10], "Check", fontColour);
	loadTexture(mTextureCache[11], "New", fontColour);
	loadTexture(mTextureCache[12], "Wrong!", fontColour);
	loadTexture(mTextureCache[13], "Right!", fontColour);

	// Load texture for start button
	loadTexture(mTextureCache[14], "Start", fontColour);
	// Load texture for pause button
	loadTexture(mTextureCache[15], "Continue", fontColour);

	// Load texture for pause game button
	loadTexture(mTextureCache[17], "Resume", fontColour);

	// Load texure for the menu
	SDL_Texture* blueTexture = NULL;
	SDL_Surface* surface = SDL_CreateRGBSurface(0, mWindowWidth, mWindowHeight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	// Set blue color for the surface
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 173, 216, 230));

	// Create texture from the surface
	blueTexture = SDL_CreateTextureFromSurface(mRenderer, surface);


	// Free surface
	SDL_FreeSurface(surface);

	// Push back texture
	mTextureCache[16] = blueTexture;






}

void Sudoku::Sudoku::createInterfaceLayout()
{
	// Define thick and thin borders
	const int thinBorder = 2;
	const int thickBorder = thinBorder + 6;

	// Treat stopwatch as a button that can't be clicked
	int buttonStartRow = 0;
	int buttonWidth = mGridWidth - 2 * thickBorder;
	// mWindowHeight = buttonHeight + 6 * thinBorder + 6 * thickBorder (rearange this equation)
	int buttonHeight = (mWindowHeight - 6 * thinBorder - 6 * thickBorder) / 11;

	buttonStartRow += thickBorder;
	int buttonStartCol = 0;
	buttonStartCol += thickBorder;
	// Set button position and dimensions
	SDL_Rect buttonRect = { buttonStartCol, buttonStartRow, buttonWidth, buttonHeight };
	mTimer.setButtonRect(buttonRect);
	// Define cell button dimensions
	// mGridWidth = 6 * thinBorder + 4 * thickBorder + 9 * buttonWidth (rearrange this equation)
	buttonWidth = (mGridWidth - 6 * thinBorder - 4 * thickBorder) / mGridCols;

	// Carry on from previous starting row
	buttonStartRow += buttonHeight;

	// Set cell button position and dimensions
	for (int gridRow = 0; gridRow < mGridRows; gridRow++)
	{
		// Add to starting row
		if (gridRow == 0) buttonStartRow += thickBorder;
		else if (gridRow % 3 == 0) buttonStartRow += buttonHeight + thickBorder;
		else buttonStartRow += buttonHeight + thinBorder;

		// Reset starting column
		int buttonStartCol = 0;

		for (int gridCol = 0; gridCol < mGridCols; gridCol++)
		{
			// Add to starting column
			if (gridCol == 0) buttonStartCol += thickBorder;
			else if (gridCol % 3 == 0) buttonStartCol += buttonWidth + thickBorder;
			else buttonStartCol += buttonWidth + thinBorder;

			// Set button position and dimensions
			SDL_Rect buttonRect = { buttonStartCol, buttonStartRow, buttonWidth, buttonHeight };
			int index = getIndex(gridRow, gridCol);
			mGrid[index].setButtonRect(buttonRect);
		}
	}

	const int numberOfOtherButtons = 3;
	mCheckButton.setTexture(mTextureCache[10]);
	mNewButton.setTexture(mTextureCache[11]);
	mPauseGameButton.setTexture(mTextureCache[17]);
	Button* otherButtons[numberOfOtherButtons] = { &mCheckButton, &mNewButton, &mPauseGameButton };

	// Redefine button width
	// mGridWidth = 4 * thickBorder + 9 * numberOfOtherButtons (rearrange this equation)
	buttonWidth = (mGridWidth - 4 * thickBorder) / numberOfOtherButtons;

	// Carry on from previous starting row
	buttonStartRow += buttonHeight + thickBorder;

	// Reset starting column
	int borderWidthTotal = 0;

	// Set check, solve, and new buttons (last row)
	for (int button = 0; button < numberOfOtherButtons; button++) // colBlock is every 3 columns of cells
	{
		// Add border width to total
		if (button == 0) borderWidthTotal += thickBorder;
		else borderWidthTotal += thickBorder;
		int buttonStartCol = button * buttonWidth + borderWidthTotal;

		// Set button position and dimensions
		SDL_Rect buttonRect = { buttonStartCol, buttonStartRow, buttonWidth, buttonHeight };
		otherButtons[button]->setButtonRect(buttonRect);
	}

	// Start button 
	SDL_Rect rect = {(mWindowWidth-200)/2,(mWindowHeight-50)/2,200,50};
	mStartButton.setButtonRect(rect);
	mStartButton.setTexture(mTextureCache[14]);

	// Pause button
	SDL_Rect rect2 = { (mWindowWidth - 200) / 2, (mWindowHeight - 50) / 2 - 100, 200, 50 };
	mPauseButton.setButtonRect(rect2);
	mPauseButton.setTexture(mTextureCache[15]);
}

void Sudoku::Sudoku::generateSudoku()
{
	// Create empty an empty grid to store generated Sudoku
	int generatedGrid[81] = { };

	// Create empty an empty grid to store solution to generated Sudoku
	int solution[81] = { };

	// Instantiate a Sudoku generator object and generate Sudoku with the empty grids
	Generator G;
	G.generate(generatedGrid, solution);

	for (int i = 0; i < 81; i++)
	{
		// Set number and solution
		mGrid[i].setNumber(generatedGrid[i]);
		mGrid[i].setSolution(solution[i]); 

		// Set editability
		if (generatedGrid[i] == 0)
		{
			// This is a cell that can be editable
			mGrid[i].setEditable(true);
		}
		else
		{
			// This cell is fixed and cannot be edited
			mGrid[i].setEditable(false);
		}

		// Set texture (0 = ' ', 1 to 9 = '1' '2'... '9')
		mGrid[i].setTexture(mTextureCache[generatedGrid[i]]);

		// Center texture onto button
		mGrid[i].centerTextureRect();

	}
}

void Sudoku::Sudoku::freeTextures()
{
	for (int i = 0; i < mTotalTextures; i++)
	{
		// Free texture if it exists
		if (mTextureCache[i] != nullptr)
		{
			SDL_DestroyTexture(mTextureCache[i]);
			mTextureCache[i] = nullptr;
		}
	}
}

void Sudoku::Sudoku::play()
{
	// Initialise SDL
	if (!initialiseSDL())
	{
		close();
	}

	// Preload textures for Sudoku grid and buttons
	preloadTextures();

	// Create interface layout
	createInterfaceLayout();

	// Generate Sudoku, set textures, and editability of each cell
	generateSudoku();

	// Play music theme on forever loop
	Mix_PlayMusic(music, -1);

	// Set first current cell selected
	Cell* currentCellSelected = &mGrid[0];
	for (int cell = 0; cell < mTotalCells; cell++)
	{
		if (mGrid[cell].isEditable())
		{
			currentCellSelected = &mGrid[cell];
			currentCellSelected->setSelected(true);
			break;
		}
	}

	// Enable text input
	SDL_StartTextInput();

	// Loop variables
	SDL_Event event;
	bool stop = false;
	bool completed = false;
	bool generateNewSudoku = false;
	bool checkSolution = false;

	// Timing for check button
	bool measureTimeForCheckButton = false;
	time_t startTimeForCheckButton;

	// Timer
	time_t startTimer;
	time(&startTimer);

	// Set default button state
	mGameState = MENU;

	while(mGameState != EXIT) {
		while (SDL_PollEvent(&event)) {

			// Handle quiting
			if (event.type == SDL_QUIT) mGameState = EXIT;

			//Handle EXIT state
			if (mGameState == EXIT) {
				stop = true;
				break;
			}

			// Handle MENU state
			if (mGameState == MENU) {
				if (mStartButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN) {
					mGameState = PLAYING;

					// Start time
					time(&startTimer);
				}
				SDL_RenderCopy(mRenderer, mTextureCache[16], NULL, NULL);
				mStartButton.centerTextureRect();
				mStartButton.renderTexture(mRenderer);

				SDL_RenderPresent(mRenderer);


			}
			// Handle PAUSED state
			if (mGameState == PAUSED) {
				if (mStartButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN) {
					mGameState = PLAYING;
					generateNewSudoku = true;
					stop = false;
				}
				if (mPauseButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN) {
					mGameState = PLAYING;
					stop = false;
				}
				SDL_RenderCopy(mRenderer, mTextureCache[16], NULL, NULL);

				// Render start button
				mStartButton.centerTextureRect();
				mStartButton.renderTexture(mRenderer);

				// Render pause button
				mPauseButton.centerTextureRect();
				mPauseButton.renderTexture(mRenderer);

				// Render present
				SDL_RenderPresent(mRenderer);
			}

			// Handle PLAYING state
			if (mGameState == PLAYING) {
				// Game loop
				while (!stop)
				{
					// Handle events on queue
					while (SDL_PollEvent(&event) != 0)
					{
						// Handle quiting and completion
						if (event.type == SDL_QUIT)
						{
							// Set stop flag
							stop = true;

							// Set game state
							mGameState = EXIT;

						}
						// Handle mouse event for "Check" button
						if (mCheckButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN)
						{
							// Set check solution flag
							checkSolution = true;

						}
						// Handle mouse event for "New" button
						if (mNewButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN)
						{
							// Set generate new Sudoku flag
							generateNewSudoku = true;

							// Play new level effect
							Mix_PlayChannel(-1, newLevelEffect, 0);
						}
						// Handle mouse event for "Pause" button
						if (mPauseGameButton.getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN)
						{
							// Set game state to paused
							mGameState = PAUSED;
							stop = true;
						}
						// Handle mouse event for cells
						for (int cell = 0; cell < mTotalCells; cell++)
						{
							// If editable
							if (mGrid[cell].isEditable())
							{
								// Set button state and return if mouse pressed on cell
								if (mGrid[cell].getMouseEvent(&event) == ButtonState::BUTTON_MOUSE_DOWN)
								{
									// Set current cell selected to false
									currentCellSelected->setSelected(false);

									// Set new cell selected to true
									currentCellSelected = &mGrid[cell];
									currentCellSelected->setSelected(true);

									// Play sound effect
									Mix_PlayChannel(-1, soundEffect, 0);
								}
							}
						}
						// Handle keyboard events for current cell selected
						currentCellSelected->handleKeyboardEvent(&event, mTextureCache);
					}
					// If "New" button was clicked
					if (generateNewSudoku)
					{
						// Generate new sudoku
						generateSudoku();

						// Set current cell selected to false
						currentCellSelected->setSelected(false);

						// Find new starting cell
						for (int cell = 0; cell < mTotalCells; cell++)
						{
							if (mGrid[cell].isEditable())
							{
								currentCellSelected = &mGrid[cell];
								currentCellSelected->setSelected(true);
								break;
							}
						}

						// Reset flags
						generateNewSudoku = false;
						completed = false;

						// Reset timer
						time(&startTimer);
					}

					// If "Check" button was clicked
					if (checkSolution)
					{
						// Check if complete
						for (int cell = 0; cell < mTotalCells; cell++)
						{
							if (!mGrid[cell].isCorrect())
							{
								completed = false;
								break;
							}
							completed = true;
						}

						for (int cell = 0;cell < mTotalCells;cell++) {
							if (mGrid[cell].isEditable()) {
								mGrid[cell].setCorrect();
							}
						}

						// Set measure time flag and starting time
						measureTimeForCheckButton = true;
						time(&startTimeForCheckButton);

						// if you win
						if (completed) {
							Mix_PlayChannel(-1, winSoundEffect, 0);
						}
						else {
							// Play check sound
							Mix_PlayChannel(-1, checkSolutionSoundEffect, 0);
						}

						// Reset flag
						checkSolution = false;
					}

					// If currently measuring time
					if (measureTimeForCheckButton)
					{
						int seconds = 2;
						if (difftime(time(NULL), startTimeForCheckButton) < seconds && completed)
						{
							// Set colour to green
							SDL_Color colour = { 91, 191, 116, SDL_ALPHA_OPAQUE };

							// Set render colour to green
							SDL_SetRenderDrawColor(mRenderer, colour.r, colour.g, colour.b, SDL_ALPHA_OPAQUE);

							// Set texture to "Right!"
							mCheckButton.setTexture(mTextureCache[13]);

							// Set mouse down colour to green
							mCheckButton.setMouseDownColour(colour);
						}
						else if (difftime(time(NULL), startTimeForCheckButton) < seconds && !completed)
						{
							// Set colour to red
							SDL_Color colour = { 200, 73, 46, SDL_ALPHA_OPAQUE };

							// Set render colour to red
							SDL_SetRenderDrawColor(mRenderer, colour.r, colour.g, colour.b, SDL_ALPHA_OPAQUE);

							// Set texture to "Wrong!"
							mCheckButton.setTexture(mTextureCache[12]);

							// Set mouse down colour to red
							mCheckButton.setMouseDownColour(colour);
						}
						else
						{
							// Reset measure time flag
							measureTimeForCheckButton = false;
						}
					}
					else
					{
						// Set texture to "Check"
						mCheckButton.setTexture(mTextureCache[10]);

						// Set render colour to black
						SDL_SetRenderDrawColor(mRenderer, mClearColour.r, mClearColour.g, mClearColour.b, mClearColour.a);
					}

					// Clear screen with rendered colour
					SDL_RenderClear(mRenderer);

					// Render buttons and texture of each cell to backbuffer
					for (int cell = 0; cell < mTotalCells; cell++)
					{
						// Render button
						mGrid[cell].renderButton(mRenderer);

						// Re-center since diffrerent numbers have different sized textures
						mGrid[cell].centerTextureRect();

						// Render texture
						mGrid[cell].renderTexture(mRenderer);
					}

					// Render check button
					mCheckButton.renderButton(mRenderer);
					mCheckButton.centerTextureRect();
					mCheckButton.renderTexture(mRenderer);

					// Render new button
					mNewButton.renderButton(mRenderer);
					mNewButton.centerTextureRect();
					mNewButton.renderTexture(mRenderer);

					// Render pause button
					mPauseGameButton.renderButton(mRenderer);
					mPauseGameButton.centerTextureRect();
					mPauseGameButton.renderTexture(mRenderer);

					// Calculate timer
					time_t difference = time(NULL) - startTimer;
					tm formattedTime;
					gmtime_s(&formattedTime, &difference);
					char timer[80];
					strftime(timer, sizeof(timer), "%H:%M:%S", &formattedTime);

					// Load and render timer (TO DO: use preloaded textures to render timer)
					SDL_Texture* timerTexture = nullptr;
					SDL_Color fontColour = { 0, 0, 0, SDL_ALPHA_OPAQUE }; // black
					loadTexture(timerTexture, timer, fontColour);
					mTimer.setTexture(timerTexture);
					mTimer.renderButton(mRenderer);
					mTimer.centerTextureRect();
					mTimer.renderTexture(mRenderer);
					SDL_DestroyTexture(timerTexture);
					timerTexture = nullptr;

					// Update screen from backbuffer and clear backbuffer
					SDL_RenderPresent(mRenderer);

					// Slow down program becuase it doesn't need to run very fast
					SDL_Delay(10);
				}

			}
		}
	}


	// Disable text input
	SDL_StopTextInput();

	// Free button textures
	freeTextures();

	// Destroy and quit
	close();
}

void Sudoku::Sudoku::close()
{
	// Destroy
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	mRenderer = nullptr;
	mWindow = nullptr;

	// Free font
	TTF_CloseFont(mFont);
	mFont = nullptr;

	// Quit
	SDL_Quit();
	TTF_Quit();
}
