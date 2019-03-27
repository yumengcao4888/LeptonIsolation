import pickle as pkl
import pathlib
import numpy as np
import torch
from torch.utils.data import DataLoader
from tensorboardX import SummaryWriter
import sys
sys.path.append("..")  # NOQA
from .Architectures.RNN import Model
from .DataStructures.LeptonTrackDataset import Torchdata, collate
from .Analyzer import plot_ROC


class RNN_Trainer:

    def __init__(self, options, leptons_with_tracks, output_folder):
        self.options = options
        self.n_events = len(leptons_with_tracks)
        self.n_training_events = int(self.options['training_split'] * self.n_events)
        self.leptons_with_tracks = leptons_with_tracks
        self.options['n_track_features'] = len(
            self.leptons_with_tracks[0][1][0])
        self.history_logger = SummaryWriter(options['output_folder'])
        self.test_truth = []
        self.test_raw_results = []

    def prepare(self):
        # split train and test
        np.random.shuffle(self.leptons_with_tracks)
        self.training_events = self.leptons_with_tracks[:self.n_training_events]
        self.test_events = self.leptons_with_tracks[self.n_training_events:]
        # prepare the generators
        self.train_set = Torchdata(self.training_events)
        self.test_set = Torchdata(self.test_events)
        # set up model
        self.model = Model(self.options)

    def make_batches(self):
        training_loader = DataLoader(
            self.train_set, batch_size=self.options['batch_size'],
            collate_fn=collate, shuffle=True, drop_last=True)
        testing_loader = DataLoader(
            self.test_set, batch_size=self.options['batch_size'],
            collate_fn=collate, shuffle=True, drop_last=True)
        return training_loader, testing_loader

    def train(self, Print=True):
        train_loss = 0
        train_acc = 0
        for epoch_n in range(self.options['n_epochs']):
            training_batches, testing_batches = self.make_batches()
            train_loss, train_acc, _, train_truth = self.model.do_train(training_batches)
            test_loss, test_acc, _, test_truth = self.model.do_eval(testing_batches)
            self.history_logger.add_scalar(
                'Accuracy/Train Accuracy', train_acc, epoch_n)
            self.history_logger.add_scalar(
                'Accuracy/Test Accuracy', test_acc, epoch_n)
            self.history_logger.add_scalar(
                'Loss/Train Loss', train_loss, epoch_n)
            self.history_logger.add_scalar(
                'Loss/Test Loss', test_loss, epoch_n)
            for name, param in self.model.named_parameters():
                self.history_logger.add_histogram(
                    name, param.clone().cpu().data.numpy(), epoch_n)

            if Print:
                print("Epoch: %03d, Train Loss: %0.4f, Train Acc: %0.4f, "
                      "Test Loss: %0.4f, Test Acc: %0.4f" % (
                          epoch_n, train_loss, train_acc, test_loss, test_acc))
        return train_loss

    def test(self, data_filename):
        self.test_set.file.reshuffle()
        _, testing_batches = self.make_batches()
        _, _, self.test_raw_results, self.test_truth = self.model.do_eval(testing_batches)
        ROC_fig = plot_ROC.plot_ROC(
            data_filename, self.test_raw_results, self.test_truth)
        self.history_logger.add_figure('ROC', ROC_fig)

    def train_and_test(self, data_filename, do_print=True, save=True):
        '''Function to run and the execute the network'''
        self.prepare()
        loss = self.train(do_print)
        self.test(data_filename)
        return loss

    def save_model(self, save_path):
        net, optimizer = self.model.get_model()
        torch.save(net, save_path + "/saved_net.pt")
        torch.save(optimizer, save_path + "/saved_optimizer.pt")
        self.history_logger.export_scalars_to_json(
            self.options["output_folder"] + "/all_scalars.json")
        self.history_logger.close()


def train(options):
    # load data
    data_filename = options['input_data']
    leptons_with_tracks = pkl.load(open(data_filename, 'rb'), encoding='latin1')
    options['lepton_size'] = len(leptons_with_tracks['lepton_labels'])
    options['track_size'] = len(leptons_with_tracks['track_labels'])
    lwt = list(
        zip(leptons_with_tracks['normed_leptons'],
            leptons_with_tracks['normed_tracks']))

    # prepare outputs
    output_folder = options['output_folder']
    if not pathlib.Path(output_folder).exists():
        pathlib.Path(output_folder).mkdir(parents=True)

    # perform training
    RNN_trainer = RNN_Trainer(options, lwt, output_folder)
    RNN_trainer.train_and_test(data_filename)

    # save results
    RNN_trainer.save_model(output_folder)