(() => {
    const app = window.SokobanWebApp = window.SokobanWebApp || {};

    function normalizeBuiltInPacks(configuredPacks) {
        if (!Array.isArray(configuredPacks)) {
            return [];
        }

        return configuredPacks.flatMap(pack => {
            if (pack === null || typeof pack !== 'object') {
                return [];
            }

            const url = typeof pack.url === 'string' ? pack.url.trim() : '';
            if (url === '') {
                return [];
            }

            const label = typeof pack.label === 'string' && pack.label.trim() !== ''
                ? pack.label.trim()
                : (url.split('/').pop() || url);
            const description = typeof pack.description === 'string' ? pack.description : '';

            return [{ label, url, description }];
        });
    }

    app.constants = {
        eventTickTime: 100,
        successDialogDelayMs: 1000,
        successAdvancePromptDelayMs: 1000,
        cameraEdgeMarginRatio: 0.2,
        resumeStorageKey: 'resume-state',
    };

    app.elements = {
        pageTitle: document.getElementById('page-title'),
        source: document.getElementById('source'),
        status: document.getElementById('status'),
        undoButton: document.getElementById('undo-button'),
        levelStats: document.getElementById('level-stats'),
        headerCopy: document.getElementById('header-copy'),
        helpButton: document.getElementById('help-button'),
        helpDialog: document.getElementById('help-dialog'),
        packButton: document.getElementById('pack-button'),
        packDialog: document.getElementById('pack-dialog'),
        packDialogClose: document.getElementById('pack-dialog-close'),
        packUrlForm: document.getElementById('pack-url-form'),
        packUrlInput: document.getElementById('pack-url-input'),
        packDropzone: document.getElementById('pack-dropzone'),
        packFileInput: document.getElementById('pack-file-input'),
        packBuiltInSection: document.getElementById('pack-builtins-section'),
        packList: document.getElementById('pack-list'),
        packDialogMessage: document.getElementById('pack-dialog-message'),
        packErrorCallout: document.getElementById('pack-error-callout'),
        packErrorDetail: document.getElementById('pack-error-detail'),
        infoButton: document.getElementById('info-button'),
        infoDialog: document.getElementById('info-dialog'),
        infoDialogTitle: document.getElementById('info-dialog-title'),
        infoContent: document.getElementById('info-content'),
        successDialog: document.getElementById('success-dialog'),
        successDialogTitle: document.getElementById('success-dialog-title'),
        successTime: document.getElementById('success-time'),
        successMoves: document.getElementById('success-moves'),
        successMessage: document.getElementById('success-message'),
        boardViewport: document.getElementById('board-viewport'),
        board: document.getElementById('game-board'),
    };

    app.runtime = {};

    app.state = {
        packText: '',
        packLabel: 'Loading pack...',
        packUrl: '',
        currentLevelIndex: 0,
        currentMoveHistory: '',
        totalLevels: 1,
        gameReady: false,
        loadingLevel: false,
        levelTimerInterval: null,
        successDialogTimer: null,
        successPromptTimer: null,
        currentPlayerTile: null,
        currentLevelTitle: '',
        currentLevelDescription: '',
        currentPackMetadata: '',
        currentMoveCount: 0,
        elapsedTimeMs: 0,
        firstMoveTimestamp: null,
        awaitingAdvanceAfterWin: false,
        gameplayFocusTimer: null,
        queuedAutoMoves: '',
        autoMoveTimer: null,
        undoRepeatTimer: null,
        undoRepeatPointerId: null,
        undoRepeatSuppressClick: false,
        lastUndoPointerDownAt: 0,
    };

    app.core = {
        builtInPacks: normalizeBuiltInPacks(window.SokobanWebConfig?.builtInPacks),

        getRequestedPackUrl() {
            return new URLSearchParams(window.location.search).get('l');
        },

        readResumeState() {
            let rawResumeState = '';

            try {
                rawResumeState = window.localStorage.getItem(app.constants.resumeStorageKey) || '';
            } catch (error) {
                console.warn('Could not read saved progress from localStorage.', error);
                return null;
            }

            if (rawResumeState === '') {
                return null;
            }

            let parsedResumeState = null;

            try {
                parsedResumeState = JSON.parse(rawResumeState);
            } catch (error) {
                app.core.clearResumeState();
                console.warn('Ignoring invalid saved progress.', error);
                return null;
            }

            if (parsedResumeState === null || typeof parsedResumeState !== 'object') {
                app.core.clearResumeState();
                return null;
            }

            const packText = typeof parsedResumeState.packText === 'string' ? parsedResumeState.packText : '';
            if (packText.trim() === '') {
                app.core.clearResumeState();
                return null;
            }

            const packLabel = typeof parsedResumeState.packLabel === 'string' && parsedResumeState.packLabel.trim() !== ''
                ? parsedResumeState.packLabel.trim()
                : 'Saved pack';
            const packUrl = typeof parsedResumeState.packUrl === 'string' ? parsedResumeState.packUrl : '';
            const initialLevelIndex = Number.isInteger(parsedResumeState.currentLevelIndex) && parsedResumeState.currentLevelIndex >= 0
                ? parsedResumeState.currentLevelIndex
                : 0;
            const moveHistory = typeof parsedResumeState.moveHistory === 'string' && /^[UDLRudlr]*$/.test(parsedResumeState.moveHistory)
                ? parsedResumeState.moveHistory
                : '';
            const elapsedTimeMs = Number.isFinite(parsedResumeState.elapsedTimeMs) && parsedResumeState.elapsedTimeMs >= 0
                ? parsedResumeState.elapsedTimeMs
                : null;

            return {
                pack: {
                    text: packText,
                    label: packLabel,
                    url: packUrl
                },
                initialLevelIndex,
                moveHistory,
                elapsedTimeMs
            };
        },

        clearResumeState() {
            try {
                window.localStorage.removeItem(app.constants.resumeStorageKey);
            } catch (error) {
                console.warn('Could not clear saved progress from localStorage.', error);
            }
        },

        saveResumeState() {
            const { state } = app;

            if (!state.gameReady || state.packText.trim() === '') {
                return;
            }
            if (typeof app.runtime.isEventOngoing === 'function' && app.runtime.isEventOngoing()) {
                return;
            }

            try {
                app.ui.syncMoveHistoryFromRuntime();
                window.localStorage.setItem(app.constants.resumeStorageKey, JSON.stringify({
                    packText: state.packText,
                    packLabel: state.packLabel,
                    packUrl: state.packUrl,
                    currentLevelIndex: state.currentLevelIndex,
                    moveHistory: state.currentMoveHistory,
                    elapsedTimeMs: state.currentMoveHistory === '' ? null : app.ui.getElapsedTimeMs()
                }));
            } catch (error) {
                console.warn('Could not save progress to localStorage.', error);
            }
        },

        settleActiveEvents(maxIterations = 4096) {
            let iterations = 0;

            while (app.runtime.isEventOngoing()) {
                app.runtime.processEvent();
                iterations += 1;

                if (iterations > maxIterations) {
                    return false;
                }
            }

            return true;
        },

        getInputCharForMoveHistory(moveChar) {
            switch (moveChar.toLowerCase()) {
                case 'u':
                    return 'w';
                case 'd':
                    return 's';
                case 'l':
                    return 'a';
                case 'r':
                    return 'd';
                default:
                    return '';
            }
        },

        replayMoveHistory(moveHistory) {
            for (const moveChar of moveHistory) {
                const inputChar = app.core.getInputCharForMoveHistory(moveChar);

                if (inputChar === '') {
                    return false;
                }
                if (!app.runtime.handleInput(inputChar.charCodeAt(0))) {
                    return false;
                }
                if (!app.core.settleActiveEvents()) {
                    return false;
                }
            }

            return true;
        },

        restoreSavedProgress(levelIndex, savedProgress) {
            const { state } = app;
            const moveHistory = typeof savedProgress?.moveHistory === 'string' ? savedProgress.moveHistory : '';
            const elapsedTimeMs = Number.isFinite(savedProgress?.elapsedTimeMs) && savedProgress.elapsedTimeMs >= 0
                ? savedProgress.elapsedTimeMs
                : 0;

            state.currentMoveHistory = '';
            state.elapsedTimeMs = 0;
            state.firstMoveTimestamp = null;

            if (moveHistory === '') {
                return true;
            }
            if (!app.core.replayMoveHistory(moveHistory)) {
                if (!app.runtime.initLevel(state.packText, levelIndex)) {
                    return false;
                }
                app.ui.syncCurrentLevelInfoFromRuntime();
                return false;
            }

            app.ui.syncMoveHistoryFromRuntime();
            if (state.currentMoveHistory !== moveHistory) {
                if (!app.runtime.initLevel(state.packText, levelIndex)) {
                    return false;
                }
                app.ui.syncCurrentLevelInfoFromRuntime();
                app.ui.syncMoveHistoryFromRuntime();
                return false;
            }
            if (state.currentMoveCount === 0) {
                state.currentMoveHistory = '';
                state.elapsedTimeMs = 0;
                state.firstMoveTimestamp = null;
                return true;
            }

            if (app.runtime.isGameWon()) {
                state.elapsedTimeMs = elapsedTimeMs;
                state.firstMoveTimestamp = null;
            } else {
                app.ui.startLevelTimer(elapsedTimeMs);
            }
            app.ui.refreshLevelStats();
            return true;
        },

        bindRuntimeApi() {
            app.runtime.initLevel = Module.cwrap('sokoban_init_web_level', 'boolean', ['string', 'number']);
            app.runtime.countLevels = Module.cwrap('sokoban_count_levels_web', 'number', ['string']);
            app.runtime.getLevelTitle = Module.cwrap('sokoban_get_level_title_web', 'string', []);
            app.runtime.getLevelDescription = Module.cwrap('sokoban_get_level_description_web', 'string', []);
            app.runtime.getPackMetadata = Module.cwrap('sokoban_get_pack_metadata_web', 'string', []);
            app.runtime.handleInput = Module.cwrap('sokoban_handle_input', 'boolean', ['number']);
            app.runtime.planTapPath = Module.cwrap('sokoban_plan_tap_path_web', 'string', ['number', 'number']);
            app.runtime.isEventOngoing = Module.cwrap('sokoban_is_event_ongoing', 'boolean', []);
            app.runtime.processEvent = Module.cwrap('sokoban_process_event', 'boolean', []);
            app.runtime.getRows = Module.cwrap('sokoban_get_rows', 'number', []);
            app.runtime.getCols = Module.cwrap('sokoban_get_cols', 'number', []);
            app.runtime.getMoveHistory = Module.cwrap('sokoban_get_move_history_web', 'string', []);
            app.runtime.getTile = Module.cwrap('sokoban_get_tile', 'number', ['number', 'number']);
            app.runtime.getInitialRows = Module.cwrap('sokoban_get_initial_rows_web', 'number', []);
            app.runtime.getInitialCols = Module.cwrap('sokoban_get_initial_cols_web', 'number', []);
            app.runtime.getInitialTile = Module.cwrap('sokoban_get_initial_tile_web', 'number', ['number', 'number']);
            app.runtime.isGameWon = Module.cwrap('sokoban_is_game_won', 'boolean', []);
        },

        applyPackToUrl(levelUrl) {
            const nextUrl = new URL(window.location.href);

            nextUrl.searchParams.set('l', levelUrl);
            window.location.assign(nextUrl.toString());
        },

        findBuiltInPackByUrl(levelUrl) {
            return app.core.builtInPacks.find(pack => pack.url === levelUrl) || null;
        },

        async fetchPackFromUrl(levelUrl, labelHint = '') {
            const resolvedUrl = new URL(levelUrl, window.location.href);
            const response = await fetch(resolvedUrl);

            if (!response.ok) {
                throw new Error(`Could not load ${resolvedUrl.href} (HTTP ${response.status}).`);
            }

            return {
                text: await response.text(),
                label: labelHint || resolvedUrl.pathname.split('/').pop() || resolvedUrl.href,
                url: resolvedUrl.href
            };
        },

        async activatePack(loadedPack, options = {}) {
            const { state } = app;
            const { clearUrlParam = false, initialLevelIndex = 0, savedProgress = null } = options;
            const detectedLevels = app.runtime.countLevels(loadedPack.text);

            if (detectedLevels <= 0) {
                throw new Error('No valid Sokoban levels were found.');
            }

            const clampedLevelIndex = Math.min(
                Math.max(Number.isInteger(initialLevelIndex) ? initialLevelIndex : 0, 0),
                detectedLevels - 1
            );

            state.packText = loadedPack.text;
            state.packLabel = loadedPack.label;
            state.packUrl = loadedPack.url;
            state.totalLevels = detectedLevels;
            state.currentLevelIndex = clampedLevelIndex;

            if (clearUrlParam) {
                const nextUrl = new URL(window.location.href);

                nextUrl.searchParams.delete('l');
                window.history.replaceState(null, '', nextUrl.toString());
            }

            if (!app.core.loadLevel(clampedLevelIndex, { savedProgress })) {
                throw new Error(`Level ${clampedLevelIndex + 1} could not be loaded.`);
            }
        },

        async loadLocalPackFile(file) {
            const { elements, state } = app;

            if (!file) {
                return;
            }

            try {
                app.ui.setPackDialogMessage('Loading local file...');
                await app.core.activatePack(
                    {
                        text: await file.text(),
                        label: file.name,
                        url: ''
                    },
                    { clearUrlParam: true }
                );
                elements.packDialog.close();
                app.ui.setPackDialogMessage('');
            } catch (error) {
                app.ui.setPackDialogMessage(error.message);
                if (!state.gameReady) {
                    app.ui.showPackErrorCallout(error.message);
                }
            }
        },

        loadLevel(levelIndex, options = {}) {
            const { state } = app;
            const { savedProgress = null } = options;
            let restoredSavedProgress = false;

            app.ui.resetLevelStats();
            state.loadingLevel = true;
            state.currentLevelIndex = levelIndex;
            app.ui.refreshHeader();
            app.ui.refreshStatus();

            if (!app.runtime.initLevel(state.packText, levelIndex)) {
                state.loadingLevel = false;
                state.gameReady = false;
                app.ui.clearCurrentLevelInfo();
                app.ui.refreshLevelInfo();
                app.ui.showPackErrorCallout(`The current pack could not load level ${levelIndex + 1}.`);
                app.ui.setStatus(`Could not load level ${levelIndex + 1}.`, 'error');
                return false;
            }

            app.ui.syncCurrentLevelInfoFromRuntime();
            if (savedProgress !== null) {
                restoredSavedProgress = app.core.restoreSavedProgress(levelIndex, savedProgress);
                if (!restoredSavedProgress) {
                    app.core.clearResumeState();
                    app.ui.resetLevelStats();
                }
            }

            state.loadingLevel = false;
            state.gameReady = true;
            if (savedProgress === null || restoredSavedProgress) {
                app.ui.syncMoveHistoryFromRuntime();
            }
            app.ui.refreshLevelInfo();
            app.ui.hidePackErrorCallout();
            app.board.drawBoard(true);
            app.ui.refreshLevelStats();
            app.ui.scheduleGameplayFocusRestore();
            app.core.saveResumeState();
            app.ui.maybeAdvanceLevel();
            return true;
        },

        async loadRequestedPack() {
            const levelParam = app.core.getRequestedPackUrl();
            const resumeState = app.core.readResumeState();

            if (levelParam === null) {
                if (resumeState !== null) {
                    return resumeState;
                }
                if (app.core.builtInPacks.length === 0) {
                    return null;
                }
                return {
                    pack: await app.core.fetchPackFromUrl(app.core.builtInPacks[0].url, app.core.builtInPacks[0].label),
                    initialLevelIndex: 0
                };
            }
            if (levelParam.trim() === '') {
                throw new Error("The 'l' query parameter is empty.");
            }

            const builtInPack = app.core.findBuiltInPackByUrl(levelParam);
            const requestedPackUrl = new URL(levelParam, window.location.href).href;
            const matchingResumeState = resumeState !== null && resumeState.pack.url === requestedPackUrl
                ? resumeState
                : null;
            let requestedPack = null;

            try {
                requestedPack = await app.core.fetchPackFromUrl(levelParam, builtInPack ? builtInPack.label : '');
            } catch (error) {
                if (matchingResumeState !== null) {
                    return matchingResumeState;
                }
                throw error;
            }

            const initialLevelIndex = matchingResumeState !== null
                ? matchingResumeState.initialLevelIndex
                : 0;

            return {
                pack: requestedPack,
                initialLevelIndex,
                moveHistory: matchingResumeState?.moveHistory || '',
                elapsedTimeMs: matchingResumeState?.elapsedTimeMs ?? null
            };
        },

        async initGame() {
            const { elements, state } = app;

            try {
                app.ui.setStatus('Loading levels...');
                app.ui.resetLevelStats();
                app.ui.hidePackErrorCallout();

                const loadTarget = await app.core.loadRequestedPack();
                if (loadTarget === null) {
                    app.ui.showPackSelectionState();
                    return;
                }

                await app.core.activatePack(loadTarget.pack, {
                    initialLevelIndex: loadTarget.initialLevelIndex,
                    savedProgress: {
                        moveHistory: loadTarget.moveHistory,
                        elapsedTimeMs: loadTarget.elapsedTimeMs
                    }
                });
            } catch (error) {
                elements.board.innerHTML = '';
                elements.source.textContent = '';
                elements.pageTitle.textContent = 'Load error';
                state.gameReady = false;
                app.ui.clearCurrentLevelInfo();
                app.ui.resetLevelStats();
                app.ui.refreshLevelInfo();
                app.ui.showPackErrorCallout(error.message);
                app.ui.setStatus(error.message, 'error');
                console.error(error);
            }
        },
    };
})();
