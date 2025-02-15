"use strict";

function combineSamples(samples) {
    let result = 0;
    for (const value of samples) {
        if (isFinite(value))
            result += value;
    }
    return result;
}

function waitForAudioProcess(scriptProcessor) {
    return new Promise(resolve => {
        scriptProcessor.addEventListener("audioprocess", event => {
            resolve(event);
        }, { once: true });
    });
}

async function testOscillatorCompressor() {
    const context = new OfflineAudioContext(1, 5000, 44100);
    const oscillator = context.createOscillator();
    oscillator.type = "triangle";
    oscillator.frequency.value = 1000;

    const compressor = context.createDynamicsCompressor();
    compressor.threshold.value = -50;
    compressor.knee.value = 40;
    compressor.ratio.value = 12;
    compressor.attack.value = 0;
    compressor.release.value = 0.2;

    oscillator.connect(compressor);
    compressor.connect(context.destination);

    oscillator.start();
    const renderedBuffer = await context.startRendering();
    oscillator.disconnect();
    compressor.disconnect();

    return combineSamples(renderedBuffer.getChannelData(0));
}

function testOscillatorCompressorWorklet() {
    return new Promise(async resolve => {
        const context = new AudioContext;

        await context.audioWorklet.addModule("fingerprint-audio-worklet.js");
        const workletNode = new AudioWorkletNode(context, "fingerprint-processor");

        let gotFingerprint = false;
        workletNode.port.onmessage = event => {
            const fingerprint = event?.data?.fingerprint || 0;
            if (!fingerprint || gotFingerprint)
                return;

            gotFingerprint = true;
            oscillator.disconnect();
            compressor.disconnect();
            workletNode.disconnect();
            resolve(fingerprint);
        };

        const oscillator = context.createOscillator();
        oscillator.type = "triangle";
        oscillator.frequency.value = 1000;

        const compressor = context.createDynamicsCompressor();
        compressor.threshold.value = -100;
        compressor.knee.value = 50;
        compressor.ratio.value = 8;
        compressor.attack.value = 0.1;
        compressor.release.value = 0.12;

        oscillator.connect(compressor).connect(workletNode).connect(context.destination);
        oscillator.start();
        context.resume();
    });
}

async function testOscillatorCompressorAnalyzer() {
    const context = new AudioContext;
    const oscillator = context.createOscillator();
    const analyzer = context.createAnalyser();
    const gain = context.createGain();
    const scriptProcessor = context.createScriptProcessor(4096, 1, 1);
    const compressor = context.createDynamicsCompressor();

    compressor.threshold.value = -50;
    compressor.knee.value = 40;
    compressor.ratio.value = 12;
    compressor.attack.value = 0;
    compressor.release.value = 0.2;

    gain.gain.value = 0;
    oscillator.type = "triangle";
    oscillator.frequency.value = 1000;
    oscillator.connect(compressor);
    compressor.connect(analyzer);
    analyzer.connect(scriptProcessor);
    scriptProcessor.connect(gain);
    gain.connect(context.destination);

    oscillator.start(0);

    await waitForAudioProcess(scriptProcessor);
    const bins = new Float32Array(analyzer.frequencyBinCount);
    analyzer.getFloatFrequencyData(bins);
    oscillator.disconnect();
    compressor.disconnect();
    analyzer.disconnect();
    scriptProcessor.disconnect();
    gain.disconnect();

    return combineSamples(bins);
}

async function testLoopingOscillatorCompressorBiquadFilter() {
    const sampleRate = 44100;
    const cloneCount = 40000;
    const targetSampleIndex = 3395;
    const stabilizationPrecision = 6.2;

    async function getBaseAudioFingerprint() {
        const baseSignal = await (async function() {
            const context = new OfflineAudioContext(1, targetSampleIndex + 1, 44100);

            const oscillator = context.createOscillator();
            oscillator.type = "square";
            oscillator.frequency.value = 1000;

            const compressor = context.createDynamicsCompressor();
            compressor.threshold.value = -70;
            compressor.knee.value = 40;
            compressor.ratio.value = 12;
            compressor.attack.value = 0;
            compressor.release.value = 0.25;

            const filter = context.createBiquadFilter();
            filter.type = "allpass";
            filter.frequency.value = 5.239622852977861;
            filter.Q.value = 0.1;

            oscillator.connect(compressor);
            compressor.connect(filter);
            filter.connect(context.destination);
            oscillator.start(0);

            return await context.startRendering();
        })();

        const context = new OfflineAudioContext(1, baseSignal.length - 1 + cloneCount, sampleRate);
        const sourceNode = context.createBufferSource();
        sourceNode.buffer = baseSignal;
        sourceNode.loop = true;
        sourceNode.loopStart = (baseSignal.length - 1) / sampleRate;
        sourceNode.loopEnd = baseSignal.length / sampleRate;
        sourceNode.connect(context.destination);
        sourceNode.start();

        const clonedSignal = await context.startRendering();
        const fingerprint = extractFingerprint(baseSignal, clonedSignal.getChannelData(0).subarray(baseSignal.length - 1));
        return Math.abs(fingerprint);
    }

    function extractFingerprint(baseSignal, clonedSample) {
        let fingerprint;
        let needsDenoising = false;

        for (let i = 0; i < clonedSample.length; i += Math.floor(clonedSample.length / 10)) {
            if (clonedSample[i] === 0) {
                // Intentionally ignored.
                continue;
            }

            if (fingerprint === undefined) {
                fingerprint = clonedSample[i];
                continue;
            }

            if (fingerprint !== clonedSample[i]) {
                needsDenoising = true;
                break;
            }
        }

        if (fingerprint === undefined)
            fingerprint = baseSignal.getChannelData(0)[baseSignal.length - 1];
        else if (needsDenoising)
            fingerprint = getMean(clonedSample);

        return fingerprint;
    }

    function getMean(signal) {
        let total = 0;
        let finiteValueCount = 0;
        for (let i = 0; i < signal.length; i++) {
            const value = signal[i];
            if (!isFinite(value))
                continue;
            total += value;
            finiteValueCount++;
        }
        return finiteValueCount ? (total / finiteValueCount) : 0;
    }

    function stabilize(value, precision) {
        if (value === 0)
            return value;
        const power = Math.floor(Math.log10(Math.abs(value)));
        const precisionPower = power - Math.floor(precision) + 1;
        const precisionBase = 10 ** -precisionPower * ((precision * 10) % 10 || 1);
        return Math.round(value * precisionBase) / precisionBase;
    }

    const rawFingerprint = await getBaseAudioFingerprint();
    return stabilize(rawFingerprint, stabilizationPrecision);
}
