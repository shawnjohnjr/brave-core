import { createAction } from 'redux-act'

export const init = createAction<void>('init')

type DataReceivedPayload = {
  feed: BraveToday.Feed | undefined
}
export const dataReceived = createAction<DataReceivedPayload>('dataReceived')

type BackgroundErrorPayload = {
  error: Error
}
export const errorGettingDataFromBackground = createAction<BackgroundErrorPayload>('errorGettingDataFromBackground')