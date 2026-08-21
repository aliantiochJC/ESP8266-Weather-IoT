/* ESP8266 Google Sheets -> PWM bridge (version 2) */

const SPREADSHEET_ID = 'PUT_YOUR_SPREADSHEET_ID_HERE';
const SHEET_NAME = 'Sheet1';
const CELL_A1 = 'A1';

// Digits only: avoids accidental spaces, Unicode characters, or copy errors.
const ACCESS_TOKEN = '73918264';
const API_VERSION = 'PWM_BRIDGE_V2';

function doGet(e) {
  const receivedToken = String(
    e && e.parameter && e.parameter.token ? e.parameter.token : ''
  ).trim();

  if (receivedToken !== ACCESS_TOKEN) {
    return jsonResponse({
      ok: false,
      error: 'unauthorized_v2',
      version: API_VERSION,
      receivedTokenLength: receivedToken.length
    });
  }

  if (SPREADSHEET_ID === 'PUT_YOUR_SPREADSHEET_ID_HERE') {
    return jsonResponse({ ok: false, error: 'spreadsheet_id_missing', version: API_VERSION });
  }

  try {
    const spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
    const sheet = spreadsheet.getSheetByName(SHEET_NAME);

    if (!sheet) {
      return jsonResponse({ ok: false, error: 'sheet_not_found', version: API_VERSION });
    }

    const value = Number(sheet.getRange(CELL_A1).getValue());
    if (!Number.isFinite(value) || value < 0 || value > 100) {
      return jsonResponse({ ok: false, error: 'value_must_be_0_to_100', version: API_VERSION });
    }

    return jsonResponse({ ok: true, value: Math.round(value), version: API_VERSION });
  } catch (error) {
    return jsonResponse({ ok: false, error: String(error), version: API_VERSION });
  }
}

function jsonResponse(data) {
  return ContentService
    .createTextOutput(JSON.stringify(data))
    .setMimeType(ContentService.MimeType.JSON);
}
