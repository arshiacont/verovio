import assert from 'node:assert/strict';
import fs from 'node:fs';
import createVerovioModule from '../dist/verovio-module.mjs';
import { VerovioToolkit } from '../dist/verovio.mjs';

const module = await createVerovioModule();
const toolkit = new VerovioToolkit(module);
const musicXml = fs.readFileSync(
  new URL('./musicxml-tuplet-display-types.xml', import.meta.url),
  'utf8',
);

toolkit.loadData(musicXml);

const measures = toolkit
  .renderToTimemap({ includeMeasures: true })
  .filter((entry) => entry.measureOn);
assert.equal(measures.length, 2);
assert.equal(measures[1].qstamp, 6);

const mei = toolkit.getMEI();
assert.match(mei, /<tuplet\b[^>]*\bnum="9"[^>]*\bnumbase="6"/);
