#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>   // For toupper()
#include <windows.h> // For audio playback and Sleep()
#include <conio.h>   // For _kbhit() and _getch()

// ================== CONSTANTS ==================
#define MAX_STRING_LENGTH 256
#define MAX_PLAYLISTS 10
#define MAX_PLAYLIST_SIZE 100
#define MAX_HISTORY_SIZE 20
#define PROGRESS_BAR_WIDTH 40
#define MASTER_PLAYLIST_FILE "playlists.txt"

// ================== STRUCTURES & ENUMS ==================
typedef struct Song {
    char title[MAX_STRING_LENGTH];
    char artist[MAX_STRING_LENGTH];
    char filePath[MAX_STRING_LENGTH];
    struct Song* next;
    struct Song* prev;
} Song;

typedef struct Playlist {
    char name[MAX_STRING_LENGTH];
    Song* head;
    Song* tail;
    int songCount;
    char filename[MAX_STRING_LENGTH];
} Playlist;

typedef enum {
    ACTION_STOP,
    ACTION_NEXT,
    ACTION_PREV,
    ACTION_FINISHED
} PlaybackAction;

// ================== GLOBAL VARIABLES ==================
Playlist playlists[MAX_PLAYLISTS];
int playlistCount = 0;
int currentPlaylistIndex = -1;
Song* songHistory[MAX_HISTORY_SIZE] = { NULL };
int historyHead = 0;

// ================== FUNCTION PROTOTYPES ==================
// --- Menus ---
void mainMenu();
void playlistManagementMenu();
void songManagementMenu();
void playbackControlsMenu();
void exitProgram();

// --- Handlers ---
void handleCreatePlaylist();
void handleSwitchPlaylist();
void handleDeletePlaylist();
void handleViewAllPlaylists();
void handleAddSong();
void handleRemoveSong();
void handleDisplaySongs();
void handleSearchSongs();
void handlePlayPlaylist();
void handlePlaySpecificSong();
void handleShuffleAndPlay();
void handleDisplayPlaybackHistory();

// --- Core Logic ---
void initializePlaylist(Playlist* playlist, const char* name);
void loadAllPlaylists();
void saveAllPlaylists();
bool loadPlaylistFromFile(Playlist* playlist);
void savePlaylistToFile(const Playlist* playlist);
void freePlaylist(Playlist* playlist);
void addSong(Playlist* playlist, const char* title, const char* artist, const char* filePath);
void removeSongFromPlaylist(Playlist* playlist, const char* title);
PlaybackAction playSongInteractive(Song* song);
void addToHistory(Song* song);
const char* stristr_custom(const char* haystack, const char* needle);
int stricmp_custom(const char* s1, const char* s2);
bool isFileNameValid(const char* name);

// ================== HELPER FUNCTIONS ==================
int getIntegerInput() {
    char buffer[100];
    int choice = -1;
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (sscanf(buffer, "%d", &choice) != 1) choice = -1;
    }
    return choice;
}

void getStringInput(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
}

void pressEnterToContinue() {
    printf("\nPress Enter to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int stricmp_custom(const char* s1, const char* s2) {
    while (*s1 && (toupper((unsigned char)*s1) == toupper((unsigned char)*s2))) {
        s1++;
        s2++;
    }
    return toupper((unsigned char)*s1) - toupper((unsigned char)*s2);
}

const char* stristr_custom(const char* haystack, const char* needle) {
    if (!*needle) return haystack;
    for (; *haystack; ++haystack) {
        if (toupper((unsigned char)*haystack) == toupper((unsigned char)*needle)) {
            const char* h = haystack, * n = needle;
            for (; *h && *n; ++h, ++n) {
                if (toupper((unsigned char)*h) != toupper((unsigned char)*n)) break;
            }
            if (!*n) return haystack;
        }
    }
    return NULL;
}

bool isFileNameValid(const char* name) {
    // Check for characters that are invalid in Windows filenames
    const char* invalidChars = "\\/:*?\"<>|";
    for (int i = 0; name[i] != '\0'; i++) {
        if (strchr(invalidChars, name[i])) {
            return false;
        }
    }
    return true;
}

// ================== PLAYLIST PERSISTENCE ==================
void loadAllPlaylists() {
    FILE* file = fopen(MASTER_PLAYLIST_FILE, "r");
    if (!file) return; // No master file exists, probably first run
    char playlistName[MAX_STRING_LENGTH];
    while (playlistCount < MAX_PLAYLISTS && fgets(playlistName, sizeof(playlistName), file)) {
        playlistName[strcspn(playlistName, "\n")] = 0;
        initializePlaylist(&playlists[playlistCount], playlistName);
        if (!loadPlaylistFromFile(&playlists[playlistCount])) {
            printf("[WARNING] Could not load data for playlist '%s'. The file may be missing or corrupted.\n", playlistName);
        }
        playlistCount++;
    }
    fclose(file);
    if (playlistCount > 0) currentPlaylistIndex = 0;
}

void saveAllPlaylists() {
    FILE* file = fopen(MASTER_PLAYLIST_FILE, "w");
    if (!file) {
        printf("[ERROR] Could not save master playlist file!\n");
        return;
    }
    for (int i = 0; i < playlistCount; i++) {
        fprintf(file, "%s\n", playlists[i].name);
        savePlaylistToFile(&playlists[i]);
    }
    fclose(file);
}

// ================== PLAYLIST & SONG MANAGEMENT ==================
void initializePlaylist(Playlist* playlist, const char* name) {
    strncpy(playlist->name, name, MAX_STRING_LENGTH - 1);
    playlist->name[MAX_STRING_LENGTH - 1] = '\0';
    snprintf(playlist->filename, MAX_STRING_LENGTH, "%s.txt", name);
    playlist->head = playlist->tail = NULL;
    playlist->songCount = 0;
}

bool loadPlaylistFromFile(Playlist* playlist) {
    FILE* file = fopen(playlist->filename, "r");
    if (!file) return false;
    char title[MAX_STRING_LENGTH], artist[MAX_STRING_LENGTH], filePath[MAX_STRING_LENGTH];
    while (fgets(title, sizeof(title), file) &&
           fgets(artist, sizeof(artist), file) &&
           fgets(filePath, sizeof(filePath), file)) {
        title[strcspn(title, "\n")] = 0;
        artist[strcspn(artist, "\n")] = 0;
        filePath[strcspn(filePath, "\n")] = 0;
        addSong(playlist, title, artist, filePath);
    }
    fclose(file);
    return true;
}

void savePlaylistToFile(const Playlist* playlist) {
    if (!playlist) return;
    FILE* file = fopen(playlist->filename, "w");
    if (!file) return;
    for (Song* current = playlist->head; current != NULL; current = current->next) {
        fprintf(file, "%s\n%s\n%s\n", current->title, current->artist, current->filePath);
    }
    fclose(file);
}

void freePlaylist(Playlist* playlist) {
    Song* current = playlist->head;
    while (current != NULL) {
        Song* next = current->next;
        free(current);
        current = next;
    }
    playlist->head = playlist->tail = NULL;
    playlist->songCount = 0;
}

void addSong(Playlist* playlist, const char* title, const char* artist, const char* filePath) {
    if (playlist->songCount >= MAX_PLAYLIST_SIZE) return;
    Song* newSong = (Song*)malloc(sizeof(Song));
    if (!newSong) {
        printf("[FATAL] Memory allocation failed. Exiting.\n");
        exit(1);
    }
    strncpy(newSong->title, title, MAX_STRING_LENGTH - 1);
    strncpy(newSong->artist, artist, MAX_STRING_LENGTH - 1);
    strncpy(newSong->filePath, filePath, MAX_STRING_LENGTH - 1);
    newSong->next = NULL;
    newSong->prev = playlist->tail;
    if (!playlist->head) playlist->head = newSong;
    else playlist->tail->next = newSong;
    playlist->tail = newSong;
    playlist->songCount++;
}

void removeSongFromPlaylist(Playlist* playlist, const char* title) {
    Song* current = playlist->head;
    while (current != NULL && stricmp_custom(current->title, title) != 0) current = current->next;
    if (!current) {
        printf("[ERROR] Song \"%s\" not found.\n", title);
        return;
    }
    if (current->prev) current->prev->next = current->next;
    else playlist->head = current->next;
    if (current->next) current->next->prev = current->prev;
    else playlist->tail = current->prev;
    free(current);
    playlist->songCount--;
    printf("[INFO] Song \"%s\" removed.\n", title);
}

void displayCurrentPlaylist() {
    if (currentPlaylistIndex == -1) {
        printf("[INFO] No playlist selected.\n");
        return;
    }
    Playlist* playlist = &playlists[currentPlaylistIndex];
    if (playlist->songCount == 0) {
        printf("[INFO] Playlist \"%s\" is empty.\n", playlist->name);
        return;
    }
    printf("\n--- Songs in: %s ---\n", playlist->name);
    int index = 1;
    for (Song* current = playlist->head; current != NULL; current = current->next) {
        printf("%d. \"%s\" by %s\n", index++, current->title, current->artist);
    }
    printf("-------------------------\n");
}

void addToHistory(Song* song) {
    if (!song) return;
    songHistory[historyHead] = song;
    historyHead = (historyHead + 1) % MAX_HISTORY_SIZE;
}

// ================== INTERACTIVE PLAYBACK CORE ==================
PlaybackAction playSongInteractive(Song* song) {
    if (!song) return ACTION_FINISHED;
    char command[MAX_STRING_LENGTH + 100], status[MAX_STRING_LENGTH];
    long totalLength = 0;
    bool isPaused = false;
    mciSendStringA("close mySound", NULL, 0, NULL);
    snprintf(command, sizeof(command), "open \"%s\" alias mySound", song->filePath);
    if (mciSendStringA(command, NULL, 0, NULL) != 0) {
        printf("\n[ERROR] Could not open/play file: %s\n", song->filePath);
        Sleep(2500);
        return ACTION_NEXT;
    }
    mciSendStringA("status mySound length", status, sizeof(status), NULL);
    totalLength = atol(status);
    if (totalLength <= 0) {
        printf("\n[ERROR] Unsupported format or zero-length file.\n");
        Sleep(2500);
        mciSendStringA("close mySound", NULL, 0, NULL);
        return ACTION_NEXT;
    }
    mciSendStringA("play mySound", NULL, 0, NULL);
    addToHistory(song);
    printf("\n\nNow Playing: \"%s\" by %s\n", song->title, song->artist);
    printf("[SPACE] Pause/Resume | [ENTER] Stop | [n] Next | [p] Previous\n");
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case ' ': isPaused = !isPaused; mciSendStringA(isPaused ? "pause mySound" : "resume mySound", NULL, 0, NULL); break;
                case '\r': mciSendStringA("close mySound", NULL, 0, NULL); return ACTION_STOP;
                case 'n': case 'N': mciSendStringA("close mySound", NULL, 0, NULL); return ACTION_NEXT;
                case 'p': case 'P': mciSendStringA("close mySound", NULL, 0, NULL); return ACTION_PREV;
            }
        }
        long currentPosition = 0;
        mciSendStringA("status mySound position", status, sizeof(status), NULL);
        currentPosition = atol(status);
        if (currentPosition >= totalLength) break;
        int totalSecs = totalLength / 1000, currentSecs = currentPosition / 1000;
        float progress = (float)currentPosition / totalLength;
        int barPos = (int)(progress * PROGRESS_BAR_WIDTH);
        printf("\r[%02d:%02d] ", currentSecs / 60, currentSecs % 60);
        for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
            if (i == barPos) printf(">");
            else printf("%c", i < barPos ? '=' : ' ');
        }
        printf(" [%02d:%02d] %s", totalSecs / 60, totalSecs % 60, isPaused ? "(Paused)" : "        ");
        fflush(stdout);
        Sleep(200);
    }
    mciSendStringA("close mySound", NULL, 0, NULL);
    return ACTION_FINISHED;
}

// ================== MENU HANDLERS ==================
void handleCreatePlaylist() {
    if (playlistCount >= MAX_PLAYLISTS) {
        printf("[ERROR] Maximum number of playlists reached.\n");
        return;
    }
    char name[MAX_STRING_LENGTH];
    printf("Enter new playlist name: ");
    getStringInput(name, sizeof(name));
    if (strlen(name) == 0) {
        printf("[ERROR] Playlist name cannot be empty.\n");
        return;
    }
    if (!isFileNameValid(name)) {
        printf("[ERROR] Playlist name contains invalid characters (e.g., \\ / : * ? \" < > |).\n");
        return;
    }
    for (int i = 0; i < playlistCount; i++) {
        if (stricmp_custom(playlists[i].name, name) == 0) {
            printf("[ERROR] A playlist with this name already exists.\n");
            return;
        }
    }
    initializePlaylist(&playlists[playlistCount], name);
    printf("[INFO] Playlist \"%s\" created.\n", name);
    currentPlaylistIndex = playlistCount;
    playlistCount++;
}

void handleSwitchPlaylist() {
    if (playlistCount == 0) {
        printf("[INFO] No playlists available.\n");
        return;
    }
    handleViewAllPlaylists();
    printf("Enter playlist number to switch to: ");
    int choice = getIntegerInput();
    if (choice > 0 && choice <= playlistCount) {
        currentPlaylistIndex = choice - 1;
        printf("[INFO] Switched to playlist \"%s\".\n", playlists[currentPlaylistIndex].name);
    } else {
        printf("[ERROR] Invalid playlist number.\n");
    }
}

void handleDeletePlaylist() {
    if (playlistCount == 0) {
        printf("[INFO] No playlists to delete.\n");
        return;
    }
    handleViewAllPlaylists();
    printf("Enter playlist number to delete: ");
    int indexToDelete = getIntegerInput() - 1;
    if (indexToDelete >= 0 && indexToDelete < playlistCount) {
        char nameToDelete[MAX_STRING_LENGTH];
        strcpy(nameToDelete, playlists[indexToDelete].name);
        remove(playlists[indexToDelete].filename);
        freePlaylist(&playlists[indexToDelete]);
        for (int i = indexToDelete; i < playlistCount - 1; i++) playlists[i] = playlists[i + 1];
        playlistCount--;
        if (currentPlaylistIndex == indexToDelete) currentPlaylistIndex = (playlistCount > 0) ? 0 : -1;
        else if (currentPlaylistIndex > indexToDelete) currentPlaylistIndex--;
        printf("[INFO] Playlist \"%s\" deleted.\n", nameToDelete);
    } else {
        printf("[ERROR] Invalid playlist number.\n");
    }
}

void handleViewAllPlaylists() {
    if (playlistCount == 0) {
        printf("[INFO] No playlists exist.\n");
        return;
    }
    printf("\n--- Available Playlists ---\n");
    for (int i = 0; i < playlistCount; i++) {
        printf("%d. %s (%d songs)\n", i + 1, playlists[i].name, playlists[i].songCount);
    }
    printf("---------------------------\n");
}

void handleAddSong() {
    if (currentPlaylistIndex == -1) {
        printf("[ERROR] Please create or switch to a playlist first.\n");
        return;
    }
    char title[MAX_STRING_LENGTH], artist[MAX_STRING_LENGTH], filePath[MAX_STRING_LENGTH];
    printf("Enter song title: ");
    getStringInput(title, sizeof(title));
    printf("Enter song artist: ");
    getStringInput(artist, sizeof(artist));
    printf("Enter song file path (e.g., C:\\Music\\song.mp3): ");
    getStringInput(filePath, sizeof(filePath));
    if (strlen(title) == 0 || strlen(artist) == 0 || strlen(filePath) == 0) {
        printf("[ERROR] All fields are required.\n");
        return;
    }
    addSong(&playlists[currentPlaylistIndex], title, artist, filePath);
    printf("[INFO] Song \"%s\" added to \"%s\".\n", title, playlists[currentPlaylistIndex].name);
}

void handleRemoveSong() {
    if (currentPlaylistIndex == -1) {
        printf("[ERROR] No playlist selected.\n");
        return;
    }
    displayCurrentPlaylist();
    if (playlists[currentPlaylistIndex].songCount == 0) return;
    char title[MAX_STRING_LENGTH];
    printf("Enter the exact title of the song to remove: ");
    getStringInput(title, sizeof(title));
    removeSongFromPlaylist(&playlists[currentPlaylistIndex], title);
}

void handleDisplaySongs() {
    displayCurrentPlaylist();
}

void handleSearchSongs() {
    if (currentPlaylistIndex == -1) {
        printf("[ERROR] Please select a playlist first.\n");
        return;
    }
    char query[MAX_STRING_LENGTH];
    printf("Enter search query (case-insensitive): ");
    getStringInput(query, sizeof(query));
    printf("\n--- Search Results in \"%s\" ---\n", playlists[currentPlaylistIndex].name);
    bool found = false;
    for (Song* current = playlists[currentPlaylistIndex].head; current != NULL; current = current->next) {
        if (stristr_custom(current->title, query) || stristr_custom(current->artist, query)) {
            printf("- \"%s\" by %s\n", current->title, current->artist);
            found = true;
        }
    }
    if (!found) printf("No songs found matching query.\n");
    printf("----------------------------------\n");
}

void handlePlayPlaylist() {
    if (currentPlaylistIndex == -1 || playlists[currentPlaylistIndex].songCount == 0) {
        printf("[INFO] Playlist is empty or not selected.\n");
        Sleep(1500);
        return;
    }
    Song* currentSong = playlists[currentPlaylistIndex].head;
    while (currentSong != NULL) {
        PlaybackAction action = playSongInteractive(currentSong);
        switch (action) {
            case ACTION_NEXT: case ACTION_FINISHED: currentSong = currentSong->next; break;
            case ACTION_PREV: currentSong = currentSong->prev ? currentSong->prev : playlists[currentPlaylistIndex].head; break;
            case ACTION_STOP: printf("\n[INFO] Playback stopped.\n"); Sleep(1500); return;
        }
    }
    printf("\n[INFO] Finished playing playlist \"%s\".\n", playlists[currentPlaylistIndex].name);
    Sleep(1500);
}

void handlePlaySpecificSong() {
    if (currentPlaylistIndex == -1 || playlists[currentPlaylistIndex].songCount == 0) {
        printf("[INFO] Playlist is empty or not selected.\n");
        Sleep(1500);
        return;
    }
    displayCurrentPlaylist();
    printf("Enter song number to play: ");
    int choice = getIntegerInput();
    if (choice > 0 && choice <= playlists[currentPlaylistIndex].songCount) {
        Song* targetSong = playlists[currentPlaylistIndex].head;
        for (int i = 1; i < choice; i++) targetSong = targetSong->next;
        Song* currentSong = targetSong;
        while (currentSong != NULL) {
            PlaybackAction action = playSongInteractive(currentSong);
            switch (action) {
                case ACTION_NEXT: case ACTION_FINISHED: currentSong = currentSong->next; break;
                case ACTION_PREV: currentSong = currentSong->prev ? currentSong->prev : playlists[currentPlaylistIndex].head; break;
                case ACTION_STOP: printf("\n[INFO] Playback stopped.\n"); Sleep(1500); return;
            }
        }
    } else {
        printf("[ERROR] Invalid song number.\n");
        Sleep(1500);
    }
}

void handleShuffleAndPlay() {
    Playlist* playlist = &playlists[currentPlaylistIndex];
    if (currentPlaylistIndex == -1 || playlist->songCount < 1) {
        printf("[INFO] Playlist is empty or not selected.\n");
        Sleep(1500);
        return;
    }
    Song** songArray = malloc(playlist->songCount * sizeof(Song*));
    if (!songArray) return;
    int i = 0;
    for (Song* current = playlist->head; current != NULL; current = current->next) songArray[i++] = current;
    srand(time(NULL));
    for (i = playlist->songCount - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Song* temp = songArray[i];
        songArray[i] = songArray[j];
        songArray[j] = temp;
    }
    printf("[INFO] Shuffling and playing playlist \"%s\".\n", playlist->name);
    for (i = 0; i < playlist->songCount; i++) {
        if (playSongInteractive(songArray[i]) == ACTION_STOP) break;
    }
    printf("\n[INFO] Shuffle play finished.\n");
    Sleep(1500);
    free(songArray);
}

void handleDisplayPlaybackHistory() {
    printf("\n--- Playback History (Most Recent First) ---\n");
    int count = 0;
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        int index = (historyHead - 1 - i + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
        if (songHistory[index] != NULL) {
            printf("%d. \"%s\" by %s\n", ++count, songHistory[index]->title, songHistory[index]->artist);
        }
    }
    if (count == 0) printf("No songs have been played yet.\n");
    printf("--------------------------------------------\n");
}

// ================== MENU LOOPS ==================
void mainMenu() {
    while (true) {
        system("cls");
        printf("\n========== MUSIC PLAYER ==========\n");
        if (currentPlaylistIndex != -1) printf("   >>> Current Playlist: %s <<<\n", playlists[currentPlaylistIndex].name);
        else printf("   >>> No Playlist Selected <<<\n");
        printf("===========================================\n");
        printf("1. Playlist Management\n");
        printf("2. Song Management\n");
        printf("3. Playback Controls\n");
        printf("4. Exit\n");
        printf("===========================================\n");
        printf("Enter your choice: ");
        int choice = getIntegerInput();
        switch (choice) {
            case 1: playlistManagementMenu(); break;
            case 2: songManagementMenu(); break;
            case 3: playbackControlsMenu(); break;
            case 4: exitProgram(); return;
            default: printf("[ERROR] Invalid choice.\n"); Sleep(1000);
        }
    }
}

void playlistManagementMenu() {
    system("cls");
    printf("\n========== PLAYLIST MANAGEMENT ==========\n");
    printf("1. Create New Playlist\n");
    printf("2. Switch To Another Playlist\n");
    printf("3. Delete A Playlist\n");
    printf("4. View All Playlists\n");
    printf("5. Back to Main Menu\n");
    printf("=========================================\n");
    printf("Enter your choice: ");
    int choice = getIntegerInput();
    switch (choice) {
        case 1: handleCreatePlaylist(); break;
        case 2: handleSwitchPlaylist(); break;
        case 3: handleDeletePlaylist(); break;
        case 4: handleViewAllPlaylists(); break;
        case 5: return;
        default: printf("[ERROR] Invalid choice.\n");
    }
    pressEnterToContinue();
}

void songManagementMenu() {
    system("cls");
    printf("\n========== SONG MANAGEMENT ==========\n");
    if (currentPlaylistIndex != -1) printf("   >>> Current Playlist: %s <<<\n", playlists[currentPlaylistIndex].name);
    printf("=====================================\n");
    printf("1. Add Song to Current Playlist\n");
    printf("2. Remove Song from Current Playlist\n");
    printf("3. Display Songs in Current Playlist\n");
    printf("4. Search for a Song\n");
    printf("5. Back to Main Menu\n");
    printf("=====================================\n");
    printf("Enter your choice: ");
    int choice = getIntegerInput();
    switch (choice) {
        case 1: handleAddSong(); break;
        case 2: handleRemoveSong(); break;
        case 3: handleDisplaySongs(); break;
        case 4: handleSearchSongs(); break;
        case 5: return;
        default: printf("[ERROR] Invalid choice.\n");
    }
    pressEnterToContinue();
}

void playbackControlsMenu() {
    system("cls");
    printf("\n========== PLAYBACK CONTROLS ==========\n");
    if (currentPlaylistIndex != -1) printf("   >>> Current Playlist: %s <<<\n", playlists[currentPlaylistIndex].name);
    printf("=======================================\n");
    printf("1. Play Current Playlist\n");
    printf("2. Play a Specific Song\n");
    printf("3. Shuffle and Play Current Playlist\n");
    printf("4. Display Playback History\n");
    printf("5. Back to Main Menu\n");
    printf("=======================================\n");
    printf("Enter your choice: ");
    int choice = getIntegerInput();
    switch (choice) {
        case 1: handlePlayPlaylist(); break;
        case 2: handlePlaySpecificSong(); break;
        case 3: handleShuffleAndPlay(); break;
        case 4: handleDisplayPlaybackHistory(); pressEnterToContinue(); break;
        case 5: return;
        default: printf("[ERROR] Invalid choice.\n"); pressEnterToContinue();
    }
}

void exitProgram() {
    printf("\n[INFO] Saving all playlists and exiting...\n");
    saveAllPlaylists();
    for (int i = 0; i < playlistCount; i++) {
        freePlaylist(&playlists[i]);
    }
    printf("Goodbye!\n");
    exit(0);
}

// ================== MAIN FUNCTION ==================
int main() {
    srand(time(NULL));
    loadAllPlaylists();
    mainMenu();
    return 0;
}

